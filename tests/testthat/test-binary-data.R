test_that("insert method works with binary data", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")

    tnt <- new(Tarantool)
    testthat::expect_true(tnt$ping())

    library(RApiSerialize)

    data(trees)
    fit <- lm(log(Volume) ~ log(Girth) + log(Height), data=trees)

    serialized <- serializeToRaw(fit)
    res <- tnt$insert("test", list(1L, serialized))
    testthat::expect_equal(length(res), 1)

    res <- tnt$select("test", list(1L), NULL)
    testthat::expect_equal(length(res), 1)
    testthat::expect_equal(length(res[[1]]), 2)

    fit2 <- unserializeFromRaw(res[[1]][[2]])
    testthat::expect_equal(fit, fit2)

    system("tarantoolctl eval example cleanup.lua")
})
