#include "FilterEditor.h"
#include "TreeFilterListWidget.h"
#include "TreeFilterHelpers.h"
#include "QtUtilities.h"
#include "QWidgetScrollListWidget.h"

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qgridlayout.h>
#include <QtWidgets/qcombobox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qpushbutton.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qscrollarea.h>

#include <algorithm>
#include <typeindex>

namespace TreeReaderApp
{
   using namespace TreeReader;
   using namespace QtAdditions;
   using namespace std;

   namespace
   {
      namespace L
      {
         inline const wchar_t* t(const wchar_t* text)
         {
            return text;
         }
      }
   }

   ////////////////////////////////////////////////////////////////////////////
   //
   // A QWidget to select and order filters.

   class FiltersEditorUI
   {
   public:
      FiltersEditorUI(FilterEditor& parent)
      : _editor(parent)
      {
         BuildUI(parent);
         ConnectUI();
      }

      TreeFilterPtr GetEdited()
      {
         UpdateEditedFromUI();
         return _edited;
      }

      wstring GetEditedName() const
      {
         return _filterName;
      }

      void SetEdited(const TreeFilterPtr& ed, const wstring& name, bool forced)
      {
         if (ed == _edited && !forced)
            return;

         _edited = ed;
         _filterName = name;

         FillUI();
      }

   private:

      void BuildUI(FilterEditor& parent)
      {
         QVBoxLayout* layout = new QVBoxLayout(&parent);
         layout->setContentsMargins(0, 0, 0, 0);

         _filterList = new TreeFilterListWidget([self = this](TreeFilterListItem* panel)
         {
            // When a filter gets deleted, update the edited filter if it is no
            // longer the root.
            delete panel;
            auto filters = self->_filterList->GetTreeFilters();
            if (filters.size() > 0)
               self->_edited = filters.back();
            else
               self->_edited = nullptr;
         });

         _filterList->setAcceptDrops(true);
         _filterList->setEnabled(true);

         _scrollFilterList = new QWidgetScrollListWidget(_filterList);
         layout->addWidget(_scrollFilterList);
      }

      void ConnectUI()
      {
      }

      static bool IsUnder(const TreeFilterPtr& filter, const TreeFilterPtr& child)
      {
         if (filter == child)
            return true;

         if (auto combine = dynamic_pointer_cast<CombineTreeFilter>(filter))
         {
            for (const auto& c : combine->Filters)
               if (IsUnder(c, child))
                  return true;
         }
         else if (auto delegate = dynamic_pointer_cast<DelegateTreeFilter>(filter))
         {
            return IsUnder(delegate->Filter, child);
         }

         return false;
      }

      void FillUI()
      {
         DisableFeedback df(_filterList, _disableFeedback);

         deque<pair<shared_ptr<CombineTreeFilter>, TreeFilterListWidget*>> combineFilters;

         _filterList->Clear();
         TreeReader::VisitFilters(_edited, true, [self = this, &combineFilters](const TreeFilterPtr& filter) -> bool
         {
            QWidgetListItem* widget = nullptr;
            for (auto& [combineFilter, combineWidget]: combineFilters)
            {
               if (IsUnder(combineFilter, filter))
               {
                  widget = combineWidget->AddTreeFilter(filter);
                  break;
               }
            }


            if (!widget)
               widget = self->_filterList->AddTreeFilter(filter);

            if (TreeFilterListItem* filterWidget = dynamic_cast<TreeFilterListItem*>(widget))
            {
               if (filterWidget->SubList)
               {
                  if (auto combine = dynamic_pointer_cast<CombineTreeFilter>(filter))
                  {
                     combineFilters.emplace_front(combine, filterWidget->SubList);
                  }
               }
            }

            return true;
         });
      }

      static TreeFilterPtr DisconnectFilter(TreeFilterPtr filter)
      {
         if (auto delegate = dynamic_pointer_cast<DelegateTreeFilter>(filter))
         {
            delegate->Filter = nullptr;
         }
         else if (auto combine = dynamic_pointer_cast<CombineTreeFilter>(filter))
         {
            combine->Filters.clear();
         }
         return filter;
      }

      static void FillFiltersFromUI(const std::vector<QWidgetListItem*>& widgets, TreeFilterPtr& root, vector<TreeFilterPtr>& previous)
      {
         for (auto& widget : widgets)
         {
            auto tfw = dynamic_cast<TreeFilterListItem*>(widget);
            if (!tfw)
               continue;
            if (!tfw->Filter)
               continue;

            auto filter = DisconnectFilter(tfw->Filter->Clone());

            if (!root)
            {
               root = filter;
            }
            else
            {
               bool placed = false;
               while (previous.size() > 0)
               {
                  if (auto delegate = dynamic_pointer_cast<DelegateTreeFilter>(previous.back()))
                  {
                     if (!delegate->Filter)
                     {
                        delegate->Filter = filter;
                        placed = true;
                        break;
                     }
                  }
                  else if (auto combine = dynamic_pointer_cast<CombineTreeFilter>(previous.back()))
                  {
                     combine->Filters.push_back(filter);
                     placed = true;
                     break;
                  }

                  previous.pop_back();
               }

               if (!placed)
               {
                  auto newRoot = And(root, filter);
                  root = newRoot;
                  previous.push_back(root);
               }
            }

            if (tfw->SubList)
            {
               vector<TreeFilterPtr> subPrevious;
               subPrevious.push_back(filter);
               FillFiltersFromUI(tfw->SubList->GetItems(), filter, subPrevious);

               // Note: when we have an explicit sub-list, we don't use it as
               //       the previous implicit combine to put other filters not
               //       under it in.
               //
               //       So we don't put it in the previous stack in this
               //       level.
            }
            else
            {
               previous.push_back(filter);
            }
         }
      }

      void UpdateEditedFromUI()
      {
         vector<TreeFilterPtr> previous;
         TreeFilterPtr root;
         FillFiltersFromUI(_filterList->GetItems(), root, previous);
         _edited = root;
      }

      FilterEditor& _editor;
      TreeFilterPtr _edited;
      wstring _filterName;

      TreeFilterListWidget* _filterList;
      QWidgetScrollListWidget* _scrollFilterList;

      int _disableFeedback = 0;
   };

   ////////////////////////////////////////////////////////////////////////////
   //
   // A QWidget to select and order filters.

   FilterEditor::FilterEditor(QWidget* parent)
   : QWidget(parent), _ui(make_unique<FiltersEditorUI>(*this))
   {
   }

   void FilterEditor::SetEdited(const TreeFilterPtr& edited, const wstring& name, bool forced)
   {
      if (!_ui)
         return;

      _ui->SetEdited(edited, name, forced);
   }

   TreeFilterPtr FilterEditor::GetEdited() const
   {
      if (!_ui)
         return {};

      return _ui->GetEdited();
   }

   wstring FilterEditor::GetEditedName() const
   {
      if (!_ui)
         return {};

      return _ui->GetEditedName();
   }
}

// vim: sw=3 : sts=3 : et : sta : 
