test_that("replace method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")
    system("tarantoolctl eval example populate_db.lua")

    tnt <- new(Tarantool)
    expect_that(tnt$ping(), is_true())

    res <- tnt$replace("test", list(1L, "eee", T, F, 3.14))
    expect_that(length(res), equals(1))
    expect_that(res[[1]], equals(list(1, "eee", TRUE, FALSE, 3.14)))

    res <- tnt$select("test", 1L, NULL)
    expect_that(length(res), equals(1))
    expect_that(res, equals(list(list(1, "eee", TRUE, FALSE, 3.14))))

    system("tarantoolctl eval example cleanup.lua")
})
