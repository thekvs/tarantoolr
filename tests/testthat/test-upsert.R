# test_that("upsert method works", {
#     system("tarantoolctl eval example cleanup.lua")
#     system("tarantoolctl eval example init.lua")
#     system("tarantoolctl eval example populate_db2.lua")
#
#     tnt <- new(Tarantool)
#     expect_that(tnt$ping(), is_true())
#
#     tnt$upsert("test", list(1L, 100L, 200L), list(index=0L, ops=list(list(field=1, op="+", arg=1))))
#     res <- tnt$select("test", 1L, NULL)
#     expect_that(length(res), equals(1))
#     expect_that(res[[1]], equals(list(1, 2, 1)))
#
#     tnt$upsert("test", list(3L, 100L, 200L), list(index=0L, ops=list(list(field=1, op="+", arg=1))))
#     res <- tnt$select("test", 3L, NULL)
#     expect_that(length(res), equals(1))
#     expect_that(res[[1]], equals(list(3, 100, 200)))
#
#     system("tarantoolctl eval example cleanup.lua")
# })
