import os
import errno
import shlex
import random
import socket
import tempfile

import yaml
import time
import shutil
import subprocess

import string

def rstring(strlen):
    return ''.join(random.choice(string.ascii_uppercase + string.digits) for _ in range(strlen))

def check_port(port, rais=True):
    try:
        sock = socket.create_connection(("127.0.0.1", port))
    except socket.error:
        return True
    if rais:
        raise RuntimeError("The server is already running on port {0}".format(port))
    return False

def find_port(port = None):
    if port is None:
        port = random.randrange(3300, 9999)
    while port < 9999:
        if check_port(port, False):
            return port
        port += 1
    return find_port(3300)


class RunnerException(object):
    pass


class TarantoolAdmin(object):
    def __init__(self, host, port):
        self.host = host
        self.port = port
        self.is_connected = False
        self.socket = None

    def connect(self):
        self.socket = socket.create_connection((self.host, self.port))
        self.socket.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        self.is_connected = True
        self.recv_exactly(128)

    def recv_exactly(self, size):
        if not self.is_connected:
            return False
        while (size > 0):
            response = self.socket.recv(size)
            size -= len(response)

    def disconnect(self):
        if self.is_connected:
            self.socket.close()
            self.socket = None
            self.is_connected = False

    def reconnect(self):
        self.disconnect()
        self.connect()

    def opt_reconnect(self):
        """ On a socket which was disconnected, recv of 0 bytes immediately
            returns with no data. On a socket which is alive, it returns EAGAIN.
            Make use of this property and detect whether or not the socket is
            dead. Reconnect a dead socket, do nothing if the socket is good."""
        try:
            if self.socket is None or self.socket.recv(0, socket.MSG_DONTWAIT) == '':
                self.reconnect()
        except socket.error as e:
            if e.errno == errno.EAGAIN:
                pass
            else:
                self.reconnect()

    def execute(self, command):
        self.opt_reconnect()
        return self.execute_no_reconnect(command)

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, type, value, tb):
        self.disconnect()

    def __call__(self, command):
        return self.execute(command)

    def execute_no_reconnect(self, command):
        if not command:
            return
        cmd = command.replace('\n', ' ') + '\n'
        self.socket.sendall(cmd)

        bufsiz = 4096
        res = ""

        while True:
            buf = self.socket.recv(bufsiz)
            if not buf:
                break
            res = res + buf
            if (res.rfind("\n...\n") >= 0 or res.rfind("\r\n...\r\n") >= 0):
                break

        return yaml.load(res)

class TarantoolServer(object):
    default_tarantool = {
            "bin":           "tarantool",
            "logfile":   "tarantool.log",
            "init":           "init.lua"}

    default_cfg = {
            "custom_proc_title": "\"tarantool-python testing\"",
            "slab_alloc_arena":                             0.5,
            "pid_file":                           "\"box.pid\"",
            "rows_per_wal":                                 200}

    @property
    def logfile_path(self):
        return os.path.join(self.vardir, self.default_tarantool['logfile'])

    @property
    def cfgfile_path(self):
        return os.path.join(self.vardir, self.default_tarantool['config'])

    @property
    def script_path(self):
        return os.path.join(self.vardir, self.default_tarantool['init'])

    @property
    def script_dst(self):
        return os.path.join(self.vardir, os.path.basename(self.script))

    @property
    def script(self):
        if not hasattr(self, '_script'): self._script = None
        return self._script
    @script.setter
    def script(self, val):
        if val is None:
            if hasattr(self, '_script'):
                delattr(self, '_script')
            return
        self._script = os.path.abspath(val)

    @property
    def binary(self):
        if not hasattr(self, '_binary'):
            self._binary = self.find_exe()
        return self._binary

    @property
    def _admin(self):
        if not hasattr(self, 'admin'):
            self.admin = None
        return self.admin
    @_admin.setter
    def _admin(self, port):
        try:
            int(port)
        except ValueError:
            raise ValueError("Bad port number: '%s'" % port)
        if hasattr(self, 'admin'):
            del self.admin
        self.admin = TarantoolAdmin('127.0.0.1', port)

    @property
    def log_des(self):
        if not hasattr(self, '_log_des'):
            self._log_des = open(self.logfile_path, 'a')
        return self._log_des
    @log_des.deleter
    def log_des(self):
        if not hasattr(self, '_log_des'):
            return
        if not self._log_des.closed:
            self._log_des.close()
        delattr(self, '_log_des')

    def __init__(self, unix = False):
        os.popen('ulimit -c unlimited')
        self.process = None
        self.args = {}
        self.vardir = tempfile.mkdtemp(prefix='var_', dir=os.getcwd())
        self.args['admin'] = find_port()
        self._admin = self.args['admin']
        if not unix:
            self.args['primary'] = find_port(self.args['admin'] + 1)
        else:
            self.args['primary'] = os.path.join('/tmp/', 'listen_%s.sock' % rstring(6))
        self.find_exe()

    def find_exe(self):
        if 'TARANTOOL_BOX_PATH' in os.environ:
            os.environ["PATH"] = os.environ["TARANTOOL_BOX_PATH"] + os.pathsep + os.environ["PATH"]

        for _dir in os.environ["PATH"].split(os.pathsep):
            exe = os.path.join(_dir, self.default_tarantool["bin"])
            if os.access(exe, os.X_OK):
                return os.path.abspath(exe)
        raise RuntimeError("Can't find server executables")

    def generate_configuration(self):
        os.putenv("PRIMARY_PORT", str(self.args['primary']))
        os.putenv("ADMIN_PORT", str(self.args['admin']))

    def prepare_args(self):
        return shlex.split(self.binary if not self.script else self.script_dst)

    def wait_until_started(self):
        """ Wait until server is started.

        Server consists of two parts:
        1) wait until server is listening on sockets
        2) wait until server tells us his status
        """

        while True:
            try:
                temp = TarantoolAdmin('127.0.0.1', self.args['admin'])
                ans = temp('box.info.status')[0]
                if ans in ('running', 'primary', 'hot_standby', 'orphan', 'loading') or ans.startswith('replica'):
                    return True
                else:
                    raise Exception("Strange output for `box.info.status`: %s" % (ans))
            except socket.error as e:
                if e.errno == errno.ECONNREFUSED:
                    time.sleep(0.1)
                    continue
                raise

    def start(self):
        # Main steps for running Tarantool\Box
        # * Find binary file          --DONE(find_exe -> binary)
        # * Create vardir             --DONE(__init__)
        # * Generate cfgfile          --DONE(generate_configuration)
        # * (MAYBE) Copy init.lua     --INSIDE
        # * Concatenate arguments and
        #   start Tarantool\Box       --DONE(prepare_args)
        # * Wait unitl Tarantool\Box
        #   started                   --DONE(wait_until_started)
        self.generate_configuration()
        if self.script:
            shutil.copy(self.script, self.script_dst)
            os.chmod(self.script_dst, 0777)
        args = self.prepare_args()
        self.process = subprocess.Popen(args,
                cwd = self.vardir,
                stdout=self.log_des,
                stderr=self.log_des)
        self.wait_until_started()

    def stop(self):
        if self.process and self.process.poll() is None:
            self.process.terminate()
            self.process.wait()

    def restart(self):
        self.stop()
        self.start()

    def clean(self):
        if isinstance(self.args['primary'], str) and self.args['primary'].find('/tmp/'):
            os.unlink(self.args['primary'])
        if os.path.exists(self.vardir):
            shutil.rmtree(self.vardir)

    def __del__(self):
        self.stop()
        self.clean()

