test_that("select method works", {
    system("tarantoolctl eval example cleanup.lua")
    system("tarantoolctl eval example init.lua")
    system("tarantoolctl eval example populate_db.lua")

    tnt <- new(Tarantool)
    expect_that(tnt$ping(), is_true())

    res <- tnt$select("test", 1L, NULL)
    expect_that(length(res), equals(1))
    expect_that(res[[1]], equals(list(1, "aaa")))

    res <- tnt$select("test", list(1L), NULL)
    expect_that(length(res), equals(1))
    expect_that(res[[1]], equals(list(1, "aaa")))

    res <- tnt$select("test", NULL, NULL)
    expect_that(length(res), equals(6))
    expect_that(res, equals(list(list(1, "aaa"), list(2, "bbb"), list(3, "ccc"), list(4, "ddd"),
                                 list(5, list(1, 2, 3)), list(10, 1, 2, list(f1=1.2, f2=T, f3=F, f4=10)))))

    res <- tnt$select("test", 2L, list(limit=2, iterator=TNT_ITER_GE))
    expect_that(length(res), equals(2))
    expect_that(res, equals(list(list(2, "bbb"), list(3, "ccc"))))

    system("tarantoolctl eval example cleanup.lua")
})
