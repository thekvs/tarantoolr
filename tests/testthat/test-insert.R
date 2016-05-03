test_that("insert method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")

    tnt <- new(Tarantool)
    expect_that(tnt$ping(), is_true())

    res <- tnt$insert("test", list(1L, 1001L, "A", T, F, list("a", 1L, 1.0, list(1, 2, list("a", "b"), 3))))
    expect_that(res[[1]], equals(list(1L, 1001L, "A", TRUE, FALSE, list("a", 1L, 1.0, list(1, 2, list("a", "b"), 3)))))

    res <- tnt$insert("test", list(2L, 3.14, "B", T, F, list(1, 2, 3)))
    expect_that(res[[1]], equals(list(2L, 3.14, "B", T, F, list(1, 2, 3))))

    res <- tnt$insert("test", list(3L, NULL, NULL))
    expect_that(res[[1]], equals(list(3L, NULL, NULL)))

    res <- tnt$select("test", 3L, NULL)
    expect_that(length(res), equals(1))
    expect_that(res[[1]], equals(list(3L, NULL, NULL)))

    expect_that(tnt$insert("test", list(1L, NULL, NULL)), throws_error())

    # We don't (yet) support factors' serialization
    testthat::expect_error(tnt$insert("test", list(2000L, sapply(list("a", "b"), as.factor))))

    system("tarantoolctl eval example cleanup.lua")
})