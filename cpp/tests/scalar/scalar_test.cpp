/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cudf/scalar/scalar.hpp>
#include <cudf/types.hpp>
#include <cudf/utilities/type_dispatcher.hpp>
#include <cudf_test/base_fixture.hpp>
#include <cudf_test/column_wrapper.hpp>
#include <cudf_test/cudf_gtest.hpp>
#include <cudf_test/type_list_utilities.hpp>
#include <cudf_test/type_lists.hpp>

#include <thrust/sequence.h>
#include <random>

template <typename T>
struct TypedScalarTest : public cudf::test::BaseFixture {
};

template <typename T>
struct TypedScalarTestWithoutFixedPoint : public cudf::test::BaseFixture {
};

TYPED_TEST_CASE(TypedScalarTest, cudf::test::FixedWidthTypes);
TYPED_TEST_CASE(TypedScalarTestWithoutFixedPoint, cudf::test::FixedWidthTypesWithoutFixedPoint);

TYPED_TEST(TypedScalarTest, DefaultValidity)
{
  using Type = cudf::device_storage_type_t<TypeParam>;
  Type value = cudf::test::make_type_param_scalar<TypeParam>(7);
  cudf::scalar_type_t<TypeParam> s(value);

  EXPECT_TRUE(s.is_valid());
  EXPECT_EQ(value, s.value());
}

TYPED_TEST(TypedScalarTest, ConstructNull)
{
  TypeParam value = cudf::test::make_type_param_scalar<TypeParam>(5);
  cudf::scalar_type_t<TypeParam> s(value, false);

  EXPECT_FALSE(s.is_valid());
}

TYPED_TEST(TypedScalarTestWithoutFixedPoint, SetValue)
{
  TypeParam value = cudf::test::make_type_param_scalar<TypeParam>(9);
  cudf::scalar_type_t<TypeParam> s;
  s.set_value(value);

  EXPECT_TRUE(s.is_valid());
  EXPECT_EQ(value, s.value());
}

TYPED_TEST(TypedScalarTestWithoutFixedPoint, SetNull)
{
  TypeParam value = cudf::test::make_type_param_scalar<TypeParam>(6);
  cudf::scalar_type_t<TypeParam> s;
  s.set_value(value);
  s.set_valid(false);

  EXPECT_FALSE(s.is_valid());
}

TYPED_TEST(TypedScalarTest, CopyConstructor)
{
  using Type = cudf::device_storage_type_t<TypeParam>;
  Type value = cudf::test::make_type_param_scalar<TypeParam>(8);
  cudf::scalar_type_t<TypeParam> s(value);
  auto s2 = s;

  EXPECT_TRUE(s2.is_valid());
  EXPECT_EQ(value, s2.value());
}

TYPED_TEST(TypedScalarTest, MoveConstructor)
{
  TypeParam value = cudf::test::make_type_param_scalar<TypeParam>(8);
  cudf::scalar_type_t<TypeParam> s(value);
  auto data_ptr = s.data();
  auto mask_ptr = s.validity_data();
  decltype(s) s2(std::move(s));

  EXPECT_EQ(mask_ptr, s2.validity_data());
  EXPECT_EQ(data_ptr, s2.data());
}

struct StringScalarTest : public cudf::test::BaseFixture {
};

TEST_F(StringScalarTest, DefaultValidity)
{
  std::string value = "test string";
  auto s            = cudf::string_scalar(value);

  EXPECT_TRUE(s.is_valid());
  EXPECT_EQ(value, s.to_string());
}

TEST_F(StringScalarTest, ConstructNull)
{
  auto s = cudf::string_scalar();

  EXPECT_FALSE(s.is_valid());
}

TEST_F(StringScalarTest, CopyConstructor)
{
  std::string value = "test_string";
  auto s            = cudf::string_scalar(value);
  auto s2           = s;

  EXPECT_TRUE(s2.is_valid());
  EXPECT_EQ(value, s2.to_string());
}

TEST_F(StringScalarTest, MoveConstructor)
{
  std::string value = "another test string";
  auto s            = cudf::string_scalar(value);
  auto data_ptr     = s.data();
  auto mask_ptr     = s.validity_data();
  decltype(s) s2(std::move(s));

  EXPECT_EQ(mask_ptr, s2.validity_data());
  EXPECT_EQ(data_ptr, s2.data());
}

struct ListScalarTest : public cudf::test::BaseFixture {
};

TEST_F(ListScalarTest, DefaultValidityNonNested)
{
  auto data = cudf::test::fixed_width_column_wrapper<int32_t>{1, 2, 3};
  auto s    = cudf::list_scalar(data);

  EXPECT_TRUE(s.is_valid());
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(data, s.view());
}

TEST_F(ListScalarTest, DefaultValidityNested)
{
  auto data = cudf::test::lists_column_wrapper<int32_t>{{1, 2}, {2}, {}, {4, 5}};
  auto s    = cudf::list_scalar(data);

  EXPECT_TRUE(s.is_valid());
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(data, s.view());
}

TEST_F(ListScalarTest, ConstructNull)
{
  auto s = cudf::list_scalar();

  EXPECT_FALSE(s.is_valid());
}

TEST_F(ListScalarTest, CopyConstructorNonNested)
{
  auto data = cudf::test::fixed_width_column_wrapper<int32_t>{1, 2, 3};
  auto s    = cudf::list_scalar(data);
  auto s2   = s;

  EXPECT_TRUE(s2.is_valid());
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(data, s2.view());
  EXPECT_NE(s.view().data<int32_t>(), s2.view().data<int32_t>());
}

TEST_F(ListScalarTest, CopyConstructorNested)
{
  auto data = cudf::test::lists_column_wrapper<int32_t>{{1, 2}, {2}, {}, {4, 5}};
  auto s    = cudf::list_scalar(data);
  auto s2   = s;

  EXPECT_TRUE(s2.is_valid());
  CUDF_TEST_EXPECT_COLUMNS_EQUAL(data, s2.view());
  EXPECT_NE(s.view().child(0).data<int32_t>(), s2.view().child(0).data<int32_t>());
  EXPECT_NE(s.view().child(1).data<int32_t>(), s2.view().child(1).data<int32_t>());
}

TEST_F(ListScalarTest, MoveConstructorNonNested)
{
  auto data     = cudf::test::fixed_width_column_wrapper<int32_t>{1, 2, 3};
  auto s        = cudf::list_scalar(data);
  auto data_ptr = s.view().data<int32_t>();
  auto mask_ptr = s.validity_data();
  decltype(s) s2(std::move(s));

  EXPECT_EQ(mask_ptr, s2.validity_data());
  EXPECT_EQ(data_ptr, s2.view().data<int32_t>());
  EXPECT_EQ(s.view().data<int32_t>(), nullptr);
}

TEST_F(ListScalarTest, MoveConstructorNested)
{
  auto data       = cudf::test::lists_column_wrapper<int32_t>{{1, 2}, {2}, {}, {4, 5}};
  auto s          = cudf::list_scalar(data);
  auto offset_ptr = s.view().child(0).data<cudf::size_type>();
  auto data_ptr   = s.view().child(1).data<int32_t>();
  auto mask_ptr   = s.validity_data();
  decltype(s) s2(std::move(s));

  EXPECT_EQ(mask_ptr, s2.validity_data());
  EXPECT_EQ(offset_ptr, s2.view().child(0).data<cudf::size_type>());
  EXPECT_EQ(data_ptr, s2.view().child(1).data<int32_t>());
  EXPECT_EQ(s.view().data<int32_t>(), nullptr);
  EXPECT_EQ(s.view().num_children(), 0);
}

CUDF_TEST_PROGRAM_MAIN()
