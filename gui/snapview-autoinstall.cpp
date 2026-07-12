#include "mapping-dialog.hpp"

#include "logic/snapview.hpp"
#include "spline/spline-widget.hpp"

#include <QCoreApplication>
#include <QEvent>
#include <QTimer>
#include <QWidget>

namespace
{

struct curve_binding final
{
    const char* object_name;
    Axis axis;
    bool alt;
};

constexpr curve_binding mapping_widgets[] = {
    { "rxconfig",     Yaw,   false },
    { "ryconfig",     Pitch, false },
    { "rzconfig",     Roll,  false },
    { "txconfig",     TX,    false },
    { "tyconfig",     TY,    false },
    { "tzconfig",     TZ,    false },

    { "rxconfig_alt", Yaw,   true  },
    { "ryconfig_alt", Pitch, true  },
    { "rzconfig_alt", Roll,  true  },
    { "txconfig_alt", TX,    true  },
    { "tyconfig_alt", TY,    true  },
    { "tzconfig_alt", TZ,    true  },
};

void register_widget_points(snapview& sv, spline_widget* widget, Axis axis, bool alt)
{
    if (!widget || axis == NonAxis)
        return;

    sv.register_curve(axis, alt, widget->points());
    widget->update();
}

void configure_mapping_dialog(mapping_dialog* dialog)
{
    if (!dialog || dialog->property("snap-view.mapping.installed").toBool())
        return;

    dialog->setProperty("snap-view.mapping.installed", true);
    snapview& sv = dialog->snapview_state();

    for (const curve_binding& binding : mapping_widgets)
    {
        spline_widget* widget = dialog->findChild<spline_widget*>(QString::fromLatin1(binding.object_name));
        if (!widget)
            continue;

        widget->set_point_label_provider([axis = binding.axis](int index)
        {
            return snapview::point_name(axis, index);
        });

        QObject::connect(widget, &spline_widget::points_changed,
                         widget,
                         [widget, &sv, axis = binding.axis, alt = binding.alt]
                         {
                             register_widget_points(sv, widget, axis, alt);
                         });

        QTimer::singleShot(0, widget, [widget, &sv, axis = binding.axis, alt = binding.alt]
        {
            register_widget_points(sv, widget, axis, alt);
        });
    }
}

class snapview_auto_installer final : public QObject
{
public:
    bool eventFilter(QObject* object, QEvent* event) override
    {
        if (!object || !event)
            return QObject::eventFilter(object, event);

        const QEvent::Type type = event->type();
        if (type != QEvent::Show && type != QEvent::ChildAdded && type != QEvent::LanguageChange)
            return QObject::eventFilter(object, event);

        auto* widget = qobject_cast<QWidget*>(object);
        if (!widget)
            return QObject::eventFilter(object, event);

        if (auto* dialog = qobject_cast<mapping_dialog*>(widget))
            QTimer::singleShot(0, dialog, [dialog] { configure_mapping_dialog(dialog); });

        return QObject::eventFilter(object, event);
    }
};

void install_snapview_auto_installer()
{
    static snapview_auto_installer installer;

    if (QCoreApplication* app = QCoreApplication::instance())
        app->installEventFilter(&installer);
}

} // namespace

Q_COREAPP_STARTUP_FUNCTION(install_snapview_auto_installer)
