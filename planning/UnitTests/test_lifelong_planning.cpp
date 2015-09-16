#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace search::lp;
using namespace search;

namespace UnitTests
{
    TEST_CLASS(test_lifelong_planning)
    {
    public:
        TEST_METHOD(cost_function)
        {
            Assert::AreEqual(1, cost());
        }

        TEST_METHOD(infinity_function)
        {
            Assert::AreEqual(2147483647, infinity());
        }

        TEST_METHOD(key)
        {
            Key key{ 42, 99 };
            Assert::AreEqual(42, key.first);
            Assert::AreEqual(99, key.second);
            Assert::IsTrue(Key{ 1, 2 } < Key{ 2, 1 });
            Assert::IsTrue(Key{ 2, 1 } < Key{ 2, 2 });
            Assert::IsTrue(Key{ 2, 2 } == Key{ 2, 2 });
        }

        TEST_METHOD(lp_coordinate)
        {
            Coordinate c{ 42, 99 };
            Assert::AreEqual(42, c.x);
            Assert::AreEqual(99, c.y);
            Assert::IsTrue(Coordinate{ 1, 1 } == Coordinate{ 1, 1 });
            Assert::IsTrue(Coordinate{ 1, 2 } != Coordinate{ 1, 1 });
            Assert::AreEqual(string{ "[x = 42, y = 99]" }, c.to_string());
            Assert::AreEqual(2044121234u, c.to_hash());
            Assert::AreEqual(2044121234u, std::hash<Coordinate>{}(c));

            //test neighbour
            {
                Coordinate c{ 1, 1 };
                decltype(c.neighbours()) expect = 
                {
                    { 0, 0 }, { 1, 0 }, { 2, 0 },
                    { 0, 1 },   /* */   { 2, 1 },
                    { 0, 2 }, { 1, 2 }, { 2, 2 }
                };

                for (auto i = 1; i != expect.size(); ++i)
                    Assert::IsTrue(expect[i] == c.neighbours()[i]);
            }

            {
                unordered_set<Coordinate> blockeds;
                blockeds.insert({ 1, 2 });
                blockeds.insert({ 1, 2 });
                blockeds.insert({ 1, 3 });
                Assert::AreEqual(2u, blockeds.size());
            }
        }

        TEST_METHOD(lp_heuristics)
        {
            Assert::AreEqual(6, HeuristcFuncs{}.at("manhattan")({ 3, 4 }, { 9, 9 }));
            Assert::AreEqual(5, HeuristcFuncs{}.at("manhattan")({ 4, 4 }, { 9, 9 }));

            Assert::AreEqual(5, HeuristcFuncs{}.at("euclidean")({ 6, 5 }, { 9, 9 }));
            Assert::AreEqual(1, HeuristcFuncs{}.at("euclidean")({ 8, 8 }, { 9, 9 }));
        }

        TEST_METHOD(lp_key)
        {
            auto ls = LpState{ { 3, 4 }, 6, 7, true };
            Assert::IsTrue(Key{ 6, 6 } == Key{ ls, HeuristcFuncs{}.at("manhattan"), { 39, 39 } });
            Assert::IsTrue(Key{ 6, 6 } == Key{ ls, HeuristcFuncs{}.at("manhattan"), { 4, 4 } });
        }

        TEST_METHOD(lp_state)
        {
            auto ls = LpState{ { 3, 4 }, 6, 7, false };
            Assert::AreEqual(3, ls.coordinate.x);
            Assert::AreEqual(4, ls.coordinate.y);
            Assert::AreEqual(6, ls.g);
            Assert::AreEqual(7, ls.r);
            Assert::IsTrue(ls == LpState{ { 3, 4 }, 6, 7 });
            Assert::IsTrue(ls == LpState{ { 3, 4 }, 6, 7, false });
            Assert::IsFalse(ls.is_blocked);
        }

        TEST_METHOD(matrix_class)
        {
            Matrix matrix{ 9, 8 };
            Assert::AreEqual(9u, matrix.rows());
            Assert::AreEqual(8u, matrix.cols());
            Assert::AreEqual(2'147'483'647, infinity());

            {
                Coordinate c = { 0, 0 };
                Assert::AreEqual(infinity(), matrix.at(c).g);
                Assert::AreEqual(infinity(), matrix.at(c).r);
                Assert::IsTrue(Coordinate{ 0, 0 } == matrix.at(c).coordinate);
            }

            {
                Coordinate c = { 2, 4 };
                Assert::AreEqual(infinity(), matrix.at(c).g);
                Assert::AreEqual(infinity(), matrix.at(c).r);
                Assert::IsTrue(c == matrix.at(c).coordinate);
            }

            {
                Coordinate c = { 4, 2 };
                Assert::AreEqual(infinity(), matrix.at(c).g);
                Assert::AreEqual(infinity(), matrix.at(c).r);
                Assert::IsTrue(c == matrix.at(c).coordinate);
            }
        }

        TEST_METHOD(lp_astar)
        {
            unordered_set<Coordinate> blockeds{ { 3, 2 }, { 14, 5 } };
            LpAstarCore lpastar{ 40, 40, { 0, 0 }, { 19, 29 }, "manhattan", blockeds };
            Assert::AreEqual(2u, lpastar.heuristics.size());
            Assert::IsTrue(Coordinate{ 19, 29 } == lpastar.goal);

            {//test blockeds
                Assert::AreEqual(true, lpastar.matrix.at({ 3, 2 }).is_blocked);
                Assert::AreEqual(true, lpastar.matrix.at({ 3, 2 }).is_blocked);
                Assert::AreEqual(false, lpastar.matrix.at({ 13, 22 }).is_blocked);
                Assert::AreEqual(false, lpastar.matrix.at({ 13, 32 }).is_blocked);
            }

            {//test matrix
                Assert::IsTrue(Coordinate{ 0, 0 } == lpastar.matrix.at(Coordinate{ 0, 0 }).coordinate);
                {
                    auto actual = lpastar.matrix.at(Coordinate{ 9, 9 }).coordinate;
                    Assert::IsTrue(Coordinate{ 9, 9 } == actual);
                }
                {
                    auto actual = lpastar.matrix.at(Coordinate{ 1, 9 }).coordinate;
                    Assert::IsTrue(Coordinate{ 1, 9 } == actual);
                }
            }

            {// test priority queue
                {
                    Assert::AreEqual(0u, lpastar.q.size());
                    lpastar.q.push(LpState{ { 3, 4 }, 6, 7, true });
                    lpastar.q.push(LpState{ { 0, 1 }, 1, 2, false });
                    lpastar.q.push(LpState{ { 3, 4 }, 5, 3, false });

                    Assert::AreEqual(3u, lpastar.q.size());
                    Assert::IsTrue(LpState{ { 0, 1 }, 1, 2, false } == lpastar.q.top()); lpastar.q.pop();
                    Assert::AreEqual(2u, lpastar.q.size());
                    Assert::IsTrue(LpState{ { 3, 4 }, 5, 3, false } == lpastar.q.top()); lpastar.q.pop();
                    Assert::AreEqual(1u, lpastar.q.size());
                    Assert::IsTrue(LpState{ { 3, 4 }, 6, 7, true } == lpastar.q.top()); lpastar.q.pop();
                    Assert::AreEqual(0u, lpastar.q.size());
                    Assert::AreEqual(true, lpastar.q.empty());
                }

                {
                    LpAstarCore lpastar{ 40, 40, { 0, 0 }, { 19, 29 }, "euclidean", blockeds };
                    lpastar.q.push(LpState{ { 3, 4 }, 6, 7 });
                    lpastar.q.push(LpState{ { 0, 1 }, 1, 2 });
                    lpastar.q.push(LpState{ { 3, 4 }, 5, 3 });

                    lpastar.q.remove(LpState{ { 3, 4 }, 5, 3 });
                    Assert::AreEqual(2u, lpastar.q.size());
                    Assert::IsTrue(LpState{ { 0, 1 }, 1, 2 } == lpastar.q.top()); 
                    lpastar.q.remove(LpState{ { 3, 4 }, 6, 7 });
                    Assert::AreEqual(1u, lpastar.q.size());
                    Assert::IsTrue(LpState{ { 0, 1 }, 1, 2 } == lpastar.q.top());
                    lpastar.q.pop();
                    Assert::AreEqual(0u, lpastar.q.size());
                    Assert::AreEqual(true, lpastar.q.empty());
                }
            }
        }
    };
}