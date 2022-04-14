// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "Luau/Parser.h"

#include "doctest.h"

using namespace Luau;

namespace Luau
{
namespace Compile
{

uint64_t modelCost(AstNode* root, AstLocal* const* vars, size_t varCount);
int computeCost(uint64_t model, const bool* varsConst, size_t varCount);

} // namespace Compile
} // namespace Luau

TEST_SUITE_BEGIN("CostModel");

static uint64_t modelFunction(const char* source)
{
    Allocator allocator;
    AstNameTable names(allocator);

    ParseResult result = Parser::parse(source, strlen(source), names, allocator);
    REQUIRE(result.root != nullptr);

    AstStatFunction* func = result.root->body.data[0]->as<AstStatFunction>();
    REQUIRE(func);

    return Luau::Compile::modelCost(func->func->body, func->func->args.data, func->func->args.size);
}

TEST_CASE("Expression")
{
    uint64_t model = modelFunction(R"(
function test(a, b, c)
    return a + (b + 1) * (b + 1) - c
end
)");

    const bool args1[] = {false, false, false};
    const bool args2[] = {false, true, false};

    CHECK_EQ(5, Luau::Compile::computeCost(model, args1, 3));
    CHECK_EQ(2, Luau::Compile::computeCost(model, args2, 3));
}

TEST_CASE("PropagateVariable")
{
    uint64_t model = modelFunction(R"(
function test(a)
    local b = a * a * a
    return b * b
end
)");

    const bool args1[] = {false};
    const bool args2[] = {true};

    CHECK_EQ(3, Luau::Compile::computeCost(model, args1, 1));
    CHECK_EQ(0, Luau::Compile::computeCost(model, args2, 1));
}

TEST_CASE("LoopAssign")
{
    uint64_t model = modelFunction(R"(
function test(a)
    for i=1,3 do
        a[i] = i
    end
end
)");

    const bool args1[] = {false};
    const bool args2[] = {true};

    // loop baseline cost is 2
    CHECK_EQ(3, Luau::Compile::computeCost(model, args1, 1));
    CHECK_EQ(3, Luau::Compile::computeCost(model, args2, 1));
}

TEST_CASE("MutableVariable")
{
    uint64_t model = modelFunction(R"(
function test(a, b)
    local x = a * a
    x += b
    return x * x
end
)");

    const bool args1[] = {false};
    const bool args2[] = {true};

    CHECK_EQ(3, Luau::Compile::computeCost(model, args1, 1));
    CHECK_EQ(2, Luau::Compile::computeCost(model, args2, 1));
}

TEST_SUITE_END();