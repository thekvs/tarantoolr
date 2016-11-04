if box.space.test then
    box.space.test:drop()
end

if box.space.test2 then
    box.space.test2:drop()
end

if box.schema.func.exists('add_two_numbers') then
    box.schema.user.revoke('guest', 'execute', 'function', 'add_two_numbers')
    box.schema.func.drop('add_two_numbers')
end
