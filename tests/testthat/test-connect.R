test_that("Tarantool class ctor connects", {
    tnt <- new(Tarantool)
    expect_that(tnt$ping(), is_true())
})

test_that("Tarantool class ctor fails", {
    expect_that(tnt <- new(Tarantool, "localhost", 33333), throws_error())
})