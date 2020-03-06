#include "QWidgetDragWidget.h"
#include "QWidgetListMimeData.h"
#include "QtUtilities.h"

#include <QtWidgets/qboxlayout.h>
#include <QtWidgets/qscrollarea.h>
#include <QtWidgets/qlayout.h>

#include <QtGui/qdrag.h>
#include <QtGui/qpainter.h>
#include <QtGui/qwindow.h>
#include <QtGui/qvalidator.h>

#include <QtGui/qevent.h>

namespace QtAdditions
{
   using namespace std;

   /////////////////////////////////////////////////////////////////////////
   //
   // Tree item panel.

   QWidgetDragWidget::QWidgetDragWidget(QWidget* parent)
   : QFrame(parent)
   {
      setBackgroundRole(QPalette::ColorRole::Base);
      setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
      setMinimumSize(QSize(20, 20));
      setFrameStyle(QFrame::Box);

      _layout = new QVBoxLayout;
      _layout->setSizeConstraint(QLayout::SetMinimumSize);
      _layout->setMargin(2);
      _layout->addStretch(0);
      setLayout(_layout);
   }

   void QWidgetDragWidget::Clear()
   {
      for (auto child : GetItems())
         delete child;
   }

   QWidgetListItem* QWidgetDragWidget::AddItem(QWidgetListItem* item, int index)
   {
      if (!item)
         return nullptr;

      if (!_layout)
         return nullptr;

      if (index < 0 || index >= _layout->count())
         index = _layout->count() - 1;

      _layout->insertWidget(index, item);

      setMinimumWidth(max(minimumWidth(), item->sizeHint().width()));
      setMinimumWidth(max(minimumWidth(), item->sizeHint().width()));

      return item;
   }

   void QWidgetDragWidget::RemoveItem(QWidgetListItem* item)
   {
      item->setParent(nullptr);
      _layout->removeWidget(item);
      _layout->update();
   }

   static void GetItems_(vector<QWidgetListItem*>& filters, const QObject* widget)
   {
      if (!widget)
         return;

      for (auto& child : widget->children())
         if (auto tfItem = dynamic_cast<QWidgetListItem*>(child))
            filters.push_back(tfItem);
         else
            GetItems_(filters, child);
   }

   vector<QWidgetListItem*> QWidgetDragWidget::GetItems() const
   {
      vector<QWidgetListItem*> widgets;

      GetItems_(widgets, this);

      sort(widgets.begin(), widgets.end(), [](QWidgetListItem* lhs, QWidgetListItem* rhs)
      {
         return lhs->pos().y() < rhs->pos().y();
      });

      return widgets;
   }

   /////////////////////////////////////////////////////////////////////////
   //
   // Drag and drop.

   QWidgetListItem* QWidgetDragWidget::CloneItem(QWidgetListItem* item) const
   {
      if (!item)
         return nullptr;

      return item->Clone();
   }

   void QWidgetDragWidget::dragEnterEvent(QDragEnterEvent* event)
   {
      const QWidgetListMimeData* mime = dynamic_cast<const QWidgetListMimeData*>(event->mimeData());
      if (!mime)
         return event->ignore();

      if (event->source() == this)
      {
         event->setDropAction(Qt::MoveAction);
         event->accept();
      }
      else
      {
         event->acceptProposedAction();
      }
   }

   void QWidgetDragWidget::dragLeaveEvent(QDragLeaveEvent* event)
   {
      event->accept();
   }

   void QWidgetDragWidget::dragMoveEvent(QDragMoveEvent* event)
   {
      const QWidgetListMimeData* mime = dynamic_cast<const QWidgetListMimeData*>(event->mimeData());
      if (!mime)
         return event->ignore();

      event->setDropAction(Qt::MoveAction);
      event->accept();
   }

   void QWidgetDragWidget::dropEvent(QDropEvent* event)
   {
      const QWidgetListMimeData* mime = dynamic_cast<const QWidgetListMimeData*>(event->mimeData());
      if (!mime)
         return event->ignore();

      const QPoint position = event->pos();
      QWidgetListItem* dropOn = FindWidgetAt(position);
      const bool dropAbove = dropOn ? (position.y() < dropOn->pos().y() + dropOn->size().height() / 2) : false;
      const int dropIndexOffset = dropAbove ? 0 : 1;
      const int dropOnIndex = dropOn ? _layout->indexOf(dropOn) : -1000;

      if (event->source() == this)
      {
         // Remove panel and insert it at correct position in list.
         auto movedWidget = mime->Widget;
         if (movedWidget && movedWidget != dropOn)
         {
            const int movedIndex = _layout->indexOf(movedWidget);
            const int newIndex = dropOnIndex + dropIndexOffset - (movedIndex < dropOnIndex ? 1 : 0);
            RemoveItem(movedWidget);
            AddItem(movedWidget, newIndex);
         }
         event->setDropAction(Qt::MoveAction);
         event->accept();
      }
      else
      {
         auto newWidget = CloneItem(mime->Widget);
         if (newWidget)
         {
            const int newIndex = dropOnIndex + dropIndexOffset;
            AddItem(newWidget, newIndex);
         }
         event->acceptProposedAction();
      }
   }

   void QWidgetDragWidget::mousePressEvent(QMouseEvent* event)
   {
      QWidgetListItem* widget = FindWidgetAt(event->pos());
      if (!widget)
         return;

      QWidgetListMimeData* mimeData = new QWidgetListMimeData;
      mimeData->Widget = widget;
      mimeData->HotSpot = event->pos() - widget->pos();
      mimeData->setData(QWidgetListMimeData::MimeType, QByteArray());

      qreal dpr = window()->windowHandle()->devicePixelRatio();
      QPixmap pixmap(widget->size() * dpr);
      pixmap.setDevicePixelRatio(dpr);
      widget->render(&pixmap);

      QDrag* drag = new QDrag(this);
      drag->setMimeData(mimeData);
      drag->setPixmap(pixmap);
      drag->setHotSpot(mimeData->HotSpot);

      Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
   }

   QWidgetListItem* QWidgetDragWidget::FindWidgetAt(const QPoint& pt) const
   {
      for (auto child = childAt(pt); child; child = child->parentWidget())
         if (auto widget = dynamic_cast<QWidgetListItem*>(child))
            return widget;

      return nullptr;
   }

}

