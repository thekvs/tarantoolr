box.schema.space.create("test", {temporary = true})
box.space.test:create_index('primary', { type = 'tree', parts = { 1, 'num' }})
