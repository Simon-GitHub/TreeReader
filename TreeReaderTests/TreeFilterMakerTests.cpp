#include "TreeReader.h"
#include "TreeReaderTestHelpers.h"
#include "CppUnitTest.h"

#include <sstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std;
using namespace TreeReader;

namespace TreeReaderTests
{
	TEST_CLASS(TreeFilterMakerTests)
	{
	public:

      TEST_METHOD(ConvertToTextAcceptFilter)
      {
         auto accept = Accept();

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \naccept [ ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<AcceptTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);
      }

      TEST_METHOD(ConvertToTextStopFilter)
      {
         auto accept = Stop();

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nstop [ ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<StopTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);
      }

      TEST_METHOD(ConvertToTextContainsFilter)
      {
         auto accept = Contains(L"\"abc\"");

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \ncontains [ \"\\\"abc\\\"\" ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<ContainsTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);
         Assert::AreEqual(L"\"abc\"", rebuilt->Contained.c_str());
      }

      TEST_METHOD(ConvertToTextRegexFilter)
      {
         auto accept = Regex(L"[abc]*");

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nregex [ \"[abc]*\" ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<RegexTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);
         Assert::AreEqual(L"[abc]*", rebuilt->RegexTextForm.c_str());
      }

      TEST_METHOD(ConvertToTextNotAcceptFilter)
      {
         auto accept = Not(Accept());

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nnot [ \n accept [ ] ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<NotTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);
         Assert::IsTrue(dynamic_pointer_cast<AcceptTreeFilter>(rebuilt->Filter) != nullptr);
      }

      TEST_METHOD(ConvertToTextOrFilter)
      {
         auto accept = Or(Contains(L"a"), Contains(L"b"));

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nor [ \n contains [ \"a\" ], \n contains [ \"b\" ] ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<OrTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);

         Assert::AreEqual<size_t>(2, rebuilt->Filters.size());

         auto lhs = dynamic_pointer_cast<ContainsTreeFilter>(rebuilt->Filters[0]);
         Assert::IsTrue(lhs != nullptr);
         Assert::AreEqual(L"a", lhs->Contained.c_str());

         auto rhs = dynamic_pointer_cast<ContainsTreeFilter>(rebuilt->Filters[1]);
         Assert::IsTrue(rhs != nullptr);
         Assert::AreEqual(L"b", rhs->Contained.c_str());
      }

      TEST_METHOD(ConvertToTextAndFilter)
      {
         auto accept = And(Contains(L"a"), Accept());

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nand [ \n contains [ \"a\" ], \n accept [ ] ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<AndTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);

         Assert::AreEqual<size_t>(2, rebuilt->Filters.size());

         auto lhs = dynamic_pointer_cast<ContainsTreeFilter>(rebuilt->Filters[0]);
         Assert::IsTrue(lhs != nullptr);
         Assert::AreEqual(L"a", lhs->Contained.c_str());

         auto rhs = dynamic_pointer_cast<AcceptTreeFilter>(rebuilt->Filters[1]);
         Assert::IsTrue(rhs != nullptr);
      }

      TEST_METHOD(ConvertToTextUnderFilter)
      {
         auto accept = Under(Contains(L"a"), true);

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nunder [ true, \n contains [ \"a\" ] ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<UnderTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);

         Assert::IsTrue(rebuilt->IncludeSelf);

         auto under = dynamic_pointer_cast<ContainsTreeFilter>(rebuilt->Filter);
         Assert::IsTrue(under != nullptr);
         Assert::AreEqual(L"a", under->Contained.c_str());
      }

      TEST_METHOD(ConvertToTextRemoveChildrenFilter)
      {
         auto accept = NoChild(Contains(L"abc"));

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nno-child [ false, \n contains [ \"abc\" ] ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<RemoveChildrenTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);

         Assert::IsFalse(rebuilt->RemoveSelf);

         auto subFilter = dynamic_pointer_cast<ContainsTreeFilter>(rebuilt->Filter);
         Assert::IsTrue(subFilter != nullptr);
         Assert::AreEqual(L"abc", subFilter->Contained.c_str());
      }

      TEST_METHOD(ConvertToTextRangeFilter)
      {
         auto accept = LevelRange(7, 9);

         const wstring text = ConvertFiltersToText(accept);

         Assert::AreEqual(L"V1: \nrange [ 7, 9 ]", text.c_str());

         auto rebuilt = dynamic_pointer_cast<LevelRangeTreeFilter>(ConvertTextToFilters(text));
         Assert::IsTrue(rebuilt != nullptr);

         Assert::AreEqual<size_t>(7, rebuilt->MinLevel);
         Assert::AreEqual<size_t>(9, rebuilt->MaxLevel);
      }

      TEST_METHOD(ConvertSimpleText)
      {
         auto filter = ConvertSimpleTextToFilters(L"( a & b & ! c ) | ( > d | ( <= 3 & * & # ) ) ");

         const wstring text = ConvertFiltersToText(filter);

         Assert::AreEqual(L"V1: \nor [ \n and [ \n  contains [ \"a\" ], \n  contains [ \"b\" ], \n  not [ \n   contains [ \"c\" ] ] ], \n or [ \n  under [ true, \n   contains [ \"d\" ] ], \n  and [ \n   range [ 0, 3 ], \n   accept [ ], \n   stop [ ] ] ] ]", text.c_str());
      }

      TEST_METHOD(ConvertSimpleTextWithIfSibling)
      {
         auto filter = ConvertSimpleTextToFilters(L"a ?= b");

         const wstring text = ConvertFiltersToText(filter);

         Assert::AreEqual(L"V1: \nand [ \n contains [ \"a\" ], \n if-sib [ \n  contains [ \"b\" ] ] ]", text.c_str());
      }

      TEST_METHOD(ConvertSimpleTextWithIfSubTree)
      {
         auto filter = ConvertSimpleTextToFilters(L"a ?> b");

         const wstring text = ConvertFiltersToText(filter);

         Assert::AreEqual(L"V1: \nand [ \n contains [ \"a\" ], \n if-sub [ \n  contains [ \"b\" ] ] ]", text.c_str());
      }

	};
}