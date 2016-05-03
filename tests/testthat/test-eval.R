test_that("call method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")

    tnt <- new(Tarantool)
    testthat::expect_true(tnt$ping())

    res <- tnt$evaluate("return 5+5", NULL)
    testthat::expect_equal(res[[1]], 10)

    system("tarantoolctl eval example cleanup.lua")
})
