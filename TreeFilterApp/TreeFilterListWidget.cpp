#include "TreeFilterListWidget.h"
#include "QtUtilities.h"

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qscrollarea.h>
#include <QtWidgets/qlayout.h>

#include <QtGui/qdrag.h>
#include <QtGui/qpainter.h>
#include <QtGui/qwindow.h>
#include <QtGui/qvalidator.h>

#include <QtGui/qevent.h>

namespace TreeReaderApp
{
   using namespace TreeReader;
   using namespace std;

   /////////////////////////////////////////////////////////////////////////
   //
   // Tree filter panel.

   TreeFilterListWidget::TreeFilterListWidget(
      DeleteCallbackFunction delCallback,
      EditCallbackFunction editCallback,
      ListModifiedCallbackFunction modifCallback,
      bool stretch, QWidget* parent)
   : QWidgetListWidget(modifCallback, stretch, parent),
     DeleteCallback(delCallback),
     EditCallback(editCallback)
   {
   }

   QWidgetListItem* TreeFilterListWidget::AddTreeFilter(const TreeFilterPtr& filter, int index)
   {
      return AddTreeFilter(filter, DeleteCallback, EditCallback, index);
   }

   QWidgetListItem* TreeFilterListWidget::AddTreeFilter(const TreeFilterPtr& filter, DeleteCallbackFunction delCallback, EditCallbackFunction editCallback, int index)
   {
      return AddItem(TreeFilterListItem::Create(filter, delCallback, editCallback), index);
   }

   vector<TreeFilterPtr> TreeFilterListWidget::GetTreeFilters() const
   {
      vector<TreeFilterPtr> filters;

      vector<QWidgetListItem*> widgets = GetItems();
      for (auto& w : widgets)
      {
         if (auto tfw = dynamic_cast<TreeFilterListItem*>(w))
         {
            if (tfw->Filter)
               filters.emplace_back(tfw->Filter);
         }
      }

      return filters;
   }

   QWidgetListItem* TreeFilterListWidget::CloneItem(QWidgetListItem* item) const
   {
      if (auto tfItem = dynamic_cast<TreeFilterListItem *>(item))
         return tfItem->Clone(DeleteCallback, EditCallback);
      else
         return QWidgetListWidget::CloneItem(item);
   }
}

