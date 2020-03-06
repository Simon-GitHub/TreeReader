#pragma once

#include "QWidgetListItem.h"

#include <QtWidgets/qframe.h>

#include <functional>

class QDragEnterEvent;
class QDropEvent;
class QMouseEvent;
class QVBoxLayout;

namespace QtAdditions
{
   struct QWidgetDragWidget : public QFrame
   {
      // Create a widget drag widget.
      QWidgetDragWidget(QWidget* parent = nullptr);

      // Clears the list panel of all filters.
      void Clear();

      // Add a widget item, with an optional deletion callback.
      QWidgetListItem* AddItem(QWidgetListItem* item, int index = -1);

      // Remove a widget item.
      void RemoveItem(QWidgetListItem* item);

      // Retrieve all widget items kept in this drag widget.
      std::vector<QWidgetListItem*> GetItems() const;

   protected:
      virtual QWidgetListItem* CloneItem(QWidgetListItem*) const;

      void dragEnterEvent(QDragEnterEvent* event) override;
      void dragLeaveEvent(QDragLeaveEvent* event) override;
      void dragMoveEvent(QDragMoveEvent* event) override;
      void dropEvent(QDropEvent* event) override;
      void mousePressEvent(QMouseEvent* event) override;

      QWidgetListItem* FindWidgetAt(const QPoint& pt) const;

      QVBoxLayout* _layout = nullptr;
   };
}
