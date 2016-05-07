box.schema.space.create("test", {temporary = true})
box.space.test:create_index('primary', { type = 'tree', parts = { 1, 'num' }})

box.schema.space.create("test2", {temporary = true})
box.space.test2:create_index('primary', { type = 'tree', parts = { 1, 'num' }})
box.space.test2:create_index('secondary', { type = 'tree', unique = false, parts = { 2, 'STR' }})

function add_two_numbers(a, b)
	return a + b
end

box.schema.func.create('add_two_numbers')
box.schema.user.grant('guest', 'execute', 'function', 'add_two_numbers')
