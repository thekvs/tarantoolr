box.space.test:drop()
box.space.test2:drop()

box.schema.user.revoke('guest', 'execute', 'function', 'add_two_numbers')
box.schema.func.drop('add_two_numbers')
