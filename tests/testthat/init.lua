box.schema.space.create("test", {temporary = true})
box.space.test:create_index('primary', { type = 'tree', parts = { 1, 'num' }})

function add_two_numbers(a, b)
	return a + b
end

box.schema.func.create('add_two_numbers')
box.schema.user.grant('guest', 'execute', 'function', 'add_two_numbers')
