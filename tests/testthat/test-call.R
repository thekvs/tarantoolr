test_that("call method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")

    tnt <- new(Tarantool)
    testthat::expect_true(tnt$ping())

    res <- tnt$call("box.info", NULL)
    testthat::expect_true(is.list(res))
    testthat::expect_gt(length(res), 0)

    testthat::expect_error(tnt$call("some.unknown.function", NULL))

    system("tarantoolctl eval example cleanup.lua")
})
