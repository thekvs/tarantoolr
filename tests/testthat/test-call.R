test_that("call method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")

    tnt <- new(Tarantool)
    testthat::expect_true(tnt$ping())

    res <- tnt$call("box.info", NULL)
    testthat::expect_true(is.list(res))
    testthat::expect_gt(length(res), 0)

    res <- tnt$call("add_two_numbers", list(1, 2))
    testthat::expect_true(is.list(res))
    testthat::expect_equal(length(res), 1)
    testthat::expect_equal(length(res[[1]]), 1)
    testthat::expect_equal(res[[1]][[1]], 3)

    testthat::expect_error(tnt$call("some.unknown.function", NULL))

    system("tarantoolctl eval example cleanup.lua")
})
