// [[Rcpp::plugins(cpp11)]]

#include <memory>
#include <string>
#include <sstream>

#include <Rcpp.h>

#include <tarantool/tarantool.h>
#include <tarantool/tnt_net.h>
#include <tarantool/tnt_opt.h>

#include <msgpack.hpp>

using TntStream = struct tnt_stream;
using TntReply = struct tnt_reply;

class TntStreamDeleter
{
public:
    void operator()(TntStream *s)
    {
        tnt_stream_free(s);
    }
};

class TntReplyDeleter
{
public:
    void operator()(TntReply *r)
    {
        tnt_reply_free(r);
        free(r);
    }
};

using TntStreamPtr = std::unique_ptr<TntStream, TntStreamDeleter>;
using TntReplyPtr = std::unique_ptr<TntReply, TntReplyDeleter>;
using TntStreamRawPtr = TntStream *;

static const std::string kDefaultHost = "localhost";
static const int kDefaultPort = 3301;
static const std::string kDefaultUser = "";
static const std::string kDefaultPassword = "";

// FIXME: implement all operators
static const std::unordered_set<char> valid_update_operators{ '+', '-', '&', '|', '^', '=', '#', '!' };

class Tarantool
{
public:
    Tarantool()
    {
        initialize(kDefaultHost, kDefaultPort, kDefaultUser, kDefaultPassword);
    }

    Tarantool(std::string host, int port)
    {
        initialize(host, port, kDefaultUser, kDefaultPassword);
    }

    Tarantool(std::string host, int port, std::string user, std::string password)
    {
        initialize(host, port, user, password);
    }

    SEXP ping()
    {
        return (ping_impl());
    }

    SEXP insert(SEXP space, SEXP tpl)
    {
        TntStreamPtr packed_tuple = pack_buffer(tpl);

        return (insert_impl(space, packed_tuple));
    }

    SEXP replace(SEXP space, SEXP tpl)
    {
        TntStreamPtr packed_tuple = pack_buffer(tpl);

        return (replace_impl(space, packed_tuple));
    }

    SEXP select(SEXP space, SEXP key, const Rcpp::List &params)
    {
        TntStreamPtr packed_key = pack_buffer(key);

        uint32_t index = 0;
        uint32_t limit = std::numeric_limits<uint32_t>::max();
        uint32_t offset = 0;
        int iterator = TNT_ITER_EQ;

        if (params.containsElementNamed("index")) {
            index = Rcpp::as<uint32_t>(params["index"]);
        }

        if (params.containsElementNamed("limit")) {
            limit = Rcpp::as<uint32_t>(params["limit"]);
        }

        if (params.containsElementNamed("offset")) {
            offset = Rcpp::as<uint32_t>(params["offset"]);
        }

        if (params.containsElementNamed("iterator")) {
            iterator = Rcpp::as<int>(params["iterator"]);
        }

        return (select_impl(space, packed_key, index, limit, offset, iterator));
    }

    SEXP delete_(SEXP space, SEXP key, const Rcpp::List &params)
    {
        TntStreamPtr packed_key = pack_buffer(key);

        uint32_t index = 0;

        if (params.containsElementNamed("index")) {
            index = Rcpp::as<uint32_t>(params["index"]);
        }

        return (delete_impl(space, packed_key, index));
    }

    SEXP update(SEXP space, SEXP tpl, const Rcpp::List &params)
    {
        TntStreamPtr packed_tuple = pack_buffer(tpl);

        uint32_t index = 0;

        if (params.containsElementNamed("index")) {
            index = Rcpp::as<uint32_t>(params["index"]);
        }

        if (!params.containsElementNamed("ops")) {
            Rcpp::stop("missed mandatory entry 'ops'");
        }

        Rcpp::List ops_desc = Rcpp::as<Rcpp::List>(params["ops"]);
        TntStreamPtr packed_ops = pack_update_ops(ops_desc);

        return (update_impl(space, packed_tuple, index, packed_ops));
    }

    SEXP upsert(SEXP space, SEXP tpl, const Rcpp::List &params)
    {
        TntStreamPtr packed_tuple = pack_buffer(tpl);

        if (!params.containsElementNamed("ops")) {
            Rcpp::stop("missed mandatory entry 'ops'");
        }

        Rcpp::List ops_desc = Rcpp::as<Rcpp::List>(params["ops"]);
        TntStreamPtr packed_ops = pack_update_ops(ops_desc);

        return (upsert_impl(space, packed_tuple, packed_ops));
    }

    SEXP call(const std::string &func, SEXP args)
    {
        TntStreamPtr packed_args = pack_buffer(args);

        return (call_impl(func, packed_args));
    }

    SEXP evaluate(const std::string &lua_statement, SEXP args)
    {
        TntStreamPtr packed_args = pack_buffer(args);

        return (evaluate_impl(lua_statement, packed_args));
    }

private:
    TntStreamPtr stream;
    msgpack::sbuffer buff;
    msgpack::sbuffer update_op_buff;

    void initialize(std::string host, int port, std::string user, std::string password);
    std::string mk_connect_uri(std::string host, int port, std::string user, std::string password);
    std::string mk_error_msg(TntStreamPtr &stream);
    void unpack_msgpack_object(const msgpack::object &obj, Rcpp::List &l);
    Rcpp::List pack_list(Rcpp::List x, msgpack::packer<msgpack::sbuffer> &pk);
    void pack_elem(Rcpp::List::iterator &it, msgpack::packer<msgpack::sbuffer> &pk);
    void unpack_array(const std::vector<msgpack::object> &v, Rcpp::List &l);
    void unpack_map(const std::map<std::string, msgpack::object> &v, Rcpp::List &l);
    SEXP read_server_reply();
    int get_space_id(SEXP space);
    TntStreamPtr pack_update_ops(const Rcpp::List &ops_desc);
    TntStreamPtr pack_buffer(SEXP tpl);
    TntStreamPtr pack_update_arg(SEXP tpl);
    void check_tnt_api_rc(int rc, const char *function_name);

    SEXP ping_impl();
    SEXP insert_impl(SEXP space, TntStreamPtr &tuple);
    SEXP replace_impl(SEXP space, TntStreamPtr &tuple);
    SEXP select_impl(SEXP space, TntStreamPtr &key, uint32_t index, uint32_t limit, uint32_t offset, int iterator);
    SEXP delete_impl(SEXP space, TntStreamPtr &key, uint32_t index);
    SEXP update_impl(SEXP space, TntStreamPtr &tuple, uint32_t index, TntStreamPtr &ops);
    SEXP upsert_impl(SEXP space, TntStreamPtr &tuple, TntStreamPtr &ops);
    SEXP call_impl(const std::string &func, TntStreamPtr &args);
    SEXP evaluate_impl(const std::string &lua_statement, TntStreamPtr &args);
};

void Tarantool::pack_elem(Rcpp::List::iterator &it, msgpack::packer<msgpack::sbuffer> &pk)
{
    switch (TYPEOF(*it)) {
    case VECSXP: {
        *it = pack_list(*it, pk);
        break;
    }
    case REALSXP: {
        auto v = Rcpp::as<double>(*it);
        pk.pack(v);
        break;
    }
    case INTSXP: {
        if (Rf_isFactor(*it)) { // factors have internal type INTSXP too
            // FIXME: deal with factors!
            Rcpp::stop("R's factors not supported yet!");
        } else {
            auto v = Rcpp::as<int64_t>(*it);
            pk.pack(v);
        }
        break;
    }
    case STRSXP: {
        auto v = Rcpp::as<std::string>(*it);
        pk.pack(v);
        break;
    }
    case NILSXP:
        pk.pack_nil();
        break;
    case LGLSXP: {
        auto v = Rcpp::as<bool>(*it);
        pk.pack(v);
        break;
    }
    case RAWSXP: {
        auto v = Rcpp::as<std::vector<uint8_t>>(*it);
        pk.pack(v);
        break;
    }
    default: {
        // See https://github.com/wch/r-source/blob/e5b21d0397c607883ff25cca379687b86933d730/src/include/Rinternals.h
        // for more details about R's internal data types
        Rcpp::stop("unsupported data type, id=%i", TYPEOF(*it));
    }
    } // switch
}

void Tarantool::unpack_msgpack_object(const msgpack::object &obj, Rcpp::List &l)
{
    if (obj.type == msgpack::type::POSITIVE_INTEGER) {
        l.push_back(obj.as<unsigned long long>());
    } else if (obj.type == msgpack::type::NEGATIVE_INTEGER) {
        auto e = -static_cast<int64_t>(std::numeric_limits<unsigned long long>::max() - obj.as<long long>() + 1);
        l.push_back(e);
    } else if (obj.type == msgpack::type::FLOAT) {
        l.push_back(obj.as<double>());
    } else if (obj.type == msgpack::type::STR) {
        l.push_back(obj.as<std::string>());
    } else if (obj.type == msgpack::type::BIN) {
        // FIXME: treat binaries as lists?
        l.push_back(obj.as<std::vector<uint8_t>>());
    } else if (obj.type == msgpack::type::NIL) {
        l.push_back(R_NilValue);
    } else if (obj.type == msgpack::type::BOOLEAN) {
        l.push_back(obj.as<bool>());
    } else {
        Rcpp::stop("unsupported msgpack object: %i", obj.type);
    }
}

void Tarantool::unpack_map(const std::map<std::string, msgpack::object> &m, Rcpp::List &l)
{
    std::vector<std::string> keys;

    keys.reserve(m.size());
    for (const auto &e : m) {
        keys.push_back(e.first);
    }

    for (const auto &e : m) {
        if (e.second.type == msgpack::type::ARRAY) {
            auto v = e.second.as<std::vector<msgpack::object>>();
            Rcpp::List sub_l;
            unpack_array(v, sub_l);
            l.push_back(sub_l);
        } else if (e.second.type == msgpack::type::MAP) {
            auto m = e.second.as<std::map<std::string, msgpack::object>>();
            Rcpp::List sub_l;
            unpack_map(m, sub_l);
            l.push_back(sub_l);
        } else {
            unpack_msgpack_object(e.second, l);
        }
    }

    l.attr("names") = Rcpp::wrap(keys);
}

void Tarantool::unpack_array(const std::vector<msgpack::object> &v, Rcpp::List &l)
{
    for (const auto &e : v) {
        if (e.type == msgpack::type::ARRAY) {
            auto v = e.as<std::vector<msgpack::object>>();
            Rcpp::List sub_l;
            unpack_array(v, sub_l);
            l.push_back(sub_l);
        } else if (e.type == msgpack::type::MAP) {
            auto m = e.as<std::map<std::string, msgpack::object>>();
            Rcpp::List sub_l;
            unpack_map(m, sub_l);
            l.push_back(sub_l);
        } else {
            unpack_msgpack_object(e, l);
        }
    }
}

Rcpp::List Tarantool::pack_list(Rcpp::List x, msgpack::packer<msgpack::sbuffer> &pk)
{
    pk.pack_array(x.size());

    for (Rcpp::List::iterator it = x.begin(); it != x.end(); ++it) {
        pack_elem(it, pk);
    }

    return (x);
}

std::string Tarantool::mk_error_msg(TntStreamPtr &stream)
{
    std::string msg = std::string("tarantool: ") + std::string(tnt_strerror(stream.get()));
    return (msg);
}

void Tarantool::initialize(std::string host, int port, std::string user, std::string password)
{
    stream = TntStreamPtr(tnt_net(nullptr));
    auto uri = mk_connect_uri(host, port, user, password);

    {
        auto err = tnt_error(stream.get());
        if (err != TNT_EOK) {
            Rcpp::stop(mk_error_msg(stream));
        }
    }

    {
        auto err = tnt_set(stream.get(), TNT_OPT_URI, uri.c_str());
        if (err != TNT_EOK) {
            Rcpp::stop(mk_error_msg(stream));
        }

        tnt_set(stream.get(), TNT_OPT_SEND_BUF, 0);
        tnt_set(stream.get(), TNT_OPT_RECV_BUF, 0);

        err = tnt_connect(stream.get());
        if (err != TNT_EOK) {
            Rcpp::stop(mk_error_msg(stream));
        }

        err = tnt_reload_schema(stream.get());
        if (err != TNT_EOK) {
            Rcpp::stop(mk_error_msg(stream));
        }
    }
}

SEXP Tarantool::ping_impl()
{
    SEXP result = Rcpp::wrap(false);

    auto rc = tnt_ping(stream.get());
    if (rc == -1) {
        Rcpp::stop(mk_error_msg(stream));
    }

    auto reply = TntReplyPtr(tnt_reply_init(NULL));
    if (!reply) {
        Rcpp::stop(mk_error_msg(stream));
    }

    rc = stream->read_reply(stream.get(), reply.get());
    if (rc == -1) {
        Rcpp::stop("tarantool: ping failed");
    }
    if (reply->code == 0) {
        result = Rcpp::wrap(true);
    } else {
        std::string error_msg;
        if (reply->error && reply->error_end) {
            error_msg = std::string(reply->error, reply->error_end - reply->error);
        } else {
            error_msg = "ping failed";
        }
        Rcpp::stop(error_msg);
    }

    return result;
}

SEXP Tarantool::insert_impl(SEXP space, TntStreamPtr &tuple)
{
    auto space_id = get_space_id(space);
    auto rc = tnt_insert(stream.get(), space_id, tuple.get());
    check_tnt_api_rc(rc, "tnt_insert()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::replace_impl(SEXP space, TntStreamPtr &tuple)
{
    auto space_id = get_space_id(space);
    auto rc = tnt_replace(stream.get(), space_id, tuple.get());
    check_tnt_api_rc(rc, "tnt_replace()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::select_impl(SEXP space, TntStreamPtr &key, uint32_t index, uint32_t limit, uint32_t offset, int iterator)
{
    auto space_id = get_space_id(space);
    auto rc = tnt_select(stream.get(), space_id, index, limit, offset, iterator, key.get());
    check_tnt_api_rc(rc, "tnt_select()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::delete_impl(SEXP space, TntStreamPtr &key, uint32_t index)
{
    auto space_id = get_space_id(space);
    auto rc = tnt_delete(stream.get(), space_id, index, key.get());
    check_tnt_api_rc(rc, "tnt_delete()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::update_impl(SEXP space, TntStreamPtr &tuple, uint32_t index, TntStreamPtr &ops)
{
    auto space_id = get_space_id(space);
    auto rc = tnt_update(stream.get(), space_id, index, tuple.get(), ops.get());
    check_tnt_api_rc(rc, "tnt_update()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::upsert_impl(SEXP space, TntStreamPtr &tuple, TntStreamPtr &ops)
{
    auto space_id = get_space_id(space);
    auto rc = tnt_upsert(stream.get(), space_id, tuple.get(), ops.get());
    check_tnt_api_rc(rc, "tnt_upsert()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::call_impl(const std::string &func, TntStreamPtr &args)
{
    auto rc = tnt_call(stream.get(), func.c_str(), func.size(), args.get());
    check_tnt_api_rc(rc, "tnt_call()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

SEXP Tarantool::evaluate_impl(const std::string &lua_statement, TntStreamPtr &args)
{
    auto rc = tnt_eval(stream.get(), lua_statement.c_str(), lua_statement.size(), args.get());
    check_tnt_api_rc(rc, "tnt_eval()");

    rc = tnt_flush(stream.get());
    check_tnt_api_rc(rc, "tnt_flush()");

    auto result = read_server_reply();

    return (result);
}

std::string Tarantool::mk_connect_uri(std::string host, int port, std::string user, std::string password)
{
    std::stringstream ss;

    if (!user.empty()) {
        ss << user;
    }

    if (!password.empty()) {
        ss << ":" << password;
    }

    if (!ss.str().empty()) {
        ss << "@";
    }

    if (!host.empty() && port > 0) {
        ss << host << ":" << port;
    }

    return ss.str();
}

SEXP Tarantool::read_server_reply()
{
    SEXP result = R_NilValue;

    auto reply = TntReplyPtr(tnt_reply_init(NULL));
    if (!reply) {
        Rcpp::stop("couldn't init tnt_reply object");
    }

    auto rc = stream->read_reply(stream.get(), reply.get());
    if (rc == -1) {
        // FIXME: need to get meaningful error message.
        Rcpp::stop("read_reply() failed");
    }
    if (reply->code != 0) {
        std::string err_msg;
        if (reply->error && reply->error_end) {
            err_msg = std::string(reply->error, reply->error_end - reply->error);
        }
        Rcpp::stop(err_msg);
    } else {
        if (reply->data && reply->data_end) {
            msgpack::unpacked unpacked;
            msgpack::unpack(unpacked, reply->data, reply->data_end - reply->data);
            msgpack::object obj(unpacked.get());

            auto v = obj.as<std::vector<msgpack::object>>();
            Rcpp::List l;
            unpack_array(v, l);

            result = Rcpp::wrap(l);
        }
    }

    return result;
}

int Tarantool::get_space_id(SEXP space)
{
    int space_id = -1;

    auto t = TYPEOF(space);
    if (t == STRSXP) {
        auto s = Rcpp::as<std::string>(space);
        space_id = tnt_get_spaceno(stream.get(), s.c_str(), s.size());
        if (space_id == -1) {
            Rcpp::stop("space '%s' doesn't exist.", s.c_str());
        }
    } else if (t == INTSXP || t == REALSXP) {
        space_id = Rcpp::as<int>(space);
    } else {
        Rcpp::stop("space must be an integer or a string");
    }

    return space_id;
}

TntStreamPtr Tarantool::pack_update_ops(const Rcpp::List &x)
{
    int rc;
    TntStreamPtr ops = TntStreamPtr(tnt_update_container(NULL));

    for (const auto &v : x) {
        if (TYPEOF(v) != VECSXP) {
            Rcpp::stop("invalid structure of update operation description.");
        }

        auto single_op = Rcpp::as<Rcpp::List>(v);

        if (!single_op.containsElementNamed("field")) {
            Rcpp::stop("missied 'field' value which is a mandatory option for update operation description.");
        }

        auto field_no = Rcpp::as<uint32_t>(single_op["field"]);

        if (!single_op.containsElementNamed("op")) {
            Rcpp::stop("missied 'op' value which is a mandatory option for update operation description.");
        }

        auto op = single_op["op"];
        auto op_type = TYPEOF(op);
        if (op_type != STRSXP) {
            Rcpp::stop("update operator must be a string value.");
        }

        auto s = Rcpp::as<std::string>(op);
        if (s.size() != 1) {
            Rcpp::stop("invalid update operator: %s", s.c_str());
        }

        auto op_value = s[0];

        if (valid_update_operators.find(op_value) == valid_update_operators.end()) {
            Rcpp::stop("invalid update operator: %c", op_value);
        }

        if (!single_op.containsElementNamed("arg")) {
            Rcpp::stop("missied 'arg' value which is a mandatory option for update operation description.");
        }

        auto arg = single_op["arg"];
        auto arg_type = TYPEOF(arg);

        if (op_value == '+' || op_value == '-') {
            if (arg_type == REALSXP) {
                auto arg_value = Rcpp::as<double>(arg);
                rc = tnt_update_arith_double(ops.get(), field_no, op_value, arg_value);
                check_tnt_api_rc(rc, "tnt_update_arith_double()");
            } else if (arg_type == INTSXP) {
                auto arg_value = Rcpp::as<int64_t>(arg);
                rc = tnt_update_arith_int(ops.get(), field_no, op_value, arg_value);
                check_tnt_api_rc(rc, "tnt_update_arith_int()");
            } else {
                Rcpp::stop("invalid data type for this type of argument");
            }
        } else if (op_value == '&' || op_value == '|' || op_value == '^') {
            if (arg_type == INTSXP) {
                auto arg_value = Rcpp::as<int64_t>(arg);
                if (arg_value < 0) {
                    Rcpp::stop("bit operator requires argument to be non negative integer");
                }
                rc = tnt_update_bit(ops.get(), field_no, op_value, arg_value);
                check_tnt_api_rc(rc, "tnt_update_bit()");
            } else {
                Rcpp::stop("invalid operator for this type of argument");
            }
        } else if (op_value == '=') {
            TntStreamPtr arg_value = pack_update_arg(arg);
            rc = tnt_update_assign(ops.get(), field_no, arg_value.get());
            check_tnt_api_rc(rc, "tnt_update_assign()");
        } else if (op_value == '#') {
            if (arg_type == INTSXP || arg_type == REALSXP) {
                auto arg_value = Rcpp::as<int64_t>(arg);
                if (arg_value < 0) {
                    Rcpp::stop("argument for delete operator must be non negative integer");
                }
                rc = tnt_update_delete(ops.get(), field_no, arg_value);
                check_tnt_api_rc(rc, "tnt_update_delete()");
            } else {
                Rcpp::stop("invalid operator for this type of argument");
            }
        } else if (op_value == '!') {
            TntStreamPtr arg_value = pack_update_arg(arg);
            rc = tnt_update_insert(ops.get(), field_no, arg_value.get());
            check_tnt_api_rc(rc, "tnt_update_insert()");
        } else {
            Rcpp::stop("unknown operator: %c", op_value);
        }
    }

    rc = tnt_update_container_close(ops.get());
    check_tnt_api_rc(rc, "tnt_update_container_close()");

    return (ops);
}

void Tarantool::check_tnt_api_rc(int rc, const char *function_name)
{
    if (rc == -1) {
        Rcpp::stop("'%s' function failed.", function_name);
    }
}

TntStreamPtr Tarantool::pack_buffer(SEXP e)
{
    Rcpp::List data;

    auto key_type = TYPEOF(e);
    if (key_type == VECSXP) {
        data = Rcpp::as<Rcpp::List>(e);
    } else if (key_type == NILSXP) {
        data = Rcpp::List::create();
    } else {
        data = Rcpp::List::create(e);
    }

    buff.clear();
    msgpack::packer<msgpack::sbuffer> pk(&buff);

    pack_list(data, pk);

    return (TntStreamPtr(tnt_object_as(NULL, const_cast<char *>(buff.data()), buff.size())));
}

TntStreamPtr Tarantool::pack_update_arg(SEXP e)
{
    update_op_buff.clear();
    msgpack::packer<msgpack::sbuffer> pk(&update_op_buff);

    Rcpp::List data;

    auto key_type = TYPEOF(e);
    if (key_type == VECSXP) {
        data = Rcpp::as<Rcpp::List>(e);
        pack_list(data, pk);
    } else {
        data = Rcpp::List::create(e);
        auto it = data.begin();
        pack_elem(it, pk);
    }

    return (TntStreamPtr(tnt_object_as(NULL, const_cast<char *>(update_op_buff.data()), update_op_buff.size())));
}

RCPP_MODULE(Tarantool)
{
    Rcpp::class_<Tarantool>("Tarantool")
        .constructor("default constructor")
        .constructor<std::string, int>("constructor with host and port")
        .constructor<std::string, int, std::string, std::string>("constructor with host, port user and password")
        .method("ping", &Tarantool::ping, "runs 'PING' command to test server state")
        .method("insert", &Tarantool::insert, "inserts data")
        .method("replace", &Tarantool::replace, "replaces data")
        .method("select", &Tarantool::select, "selects data")
        .method("delete", &Tarantool::delete_, "deletes data")
        .method("update", &Tarantool::update, "selects data")
        .method("upsert", &Tarantool::upsert, "upserts data")
        .method("call", &Tarantool::call, "call lua function")
        .method("evaluate", &Tarantool::evaluate, "evaluate lua statement");
}

// [[Rcpp::export]]
void exportTarantoolConstants()
{
    Rcpp::Environment env = Rcpp::Environment::global_env();

    env["TNT_ITER_EQ"] = static_cast<int>(TNT_ITER_EQ);
    env["TNT_ITER_REQ"] = static_cast<int>(TNT_ITER_REQ);
    env["TNT_ITER_ALL"] = static_cast<int>(TNT_ITER_ALL);
    env["TNT_ITER_LT"] = static_cast<int>(TNT_ITER_LT);
    env["TNT_ITER_LE"] = static_cast<int>(TNT_ITER_LE);
    env["TNT_ITER_GE"] = static_cast<int>(TNT_ITER_GE);
    env["TNT_ITER_GT"] = static_cast<int>(TNT_ITER_GT);
    env["TNT_ITER_BITS_ALL_SET"] = static_cast<int>(TNT_ITER_BITS_ALL_SET);
    env["TNT_ITER_BITS_ANY_SET"] = static_cast<int>(TNT_ITER_BITS_ANY_SET);
    env["TNT_ITER_BITS_ALL_NOT_SET"] = static_cast<int>(TNT_ITER_BITS_ALL_NOT_SET);
    env["TNT_ITER_OVERLAP"] = static_cast<int>(TNT_ITER_OVERLAP);
    env["TNT_ITER_NEIGHBOR"] = static_cast<int>(TNT_ITER_NEIGHBOR);
}
