/****************************************************************************
** Meta object code from reading C++ file 'animpropertybrowser.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "animpropertybrowser.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'animpropertybrowser.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_netanim__AnimPropertyBroswer_t {
    QByteArrayData data[8];
    char stringdata0[108];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_netanim__AnimPropertyBroswer_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_netanim__AnimPropertyBroswer_t qt_meta_stringdata_netanim__AnimPropertyBroswer = {
    {
QT_MOC_LITERAL(0, 0, 28), // "netanim::AnimPropertyBroswer"
QT_MOC_LITERAL(1, 29, 18), // "nodeIdSelectorSlot"
QT_MOC_LITERAL(2, 48, 0), // ""
QT_MOC_LITERAL(3, 49, 8), // "newIndex"
QT_MOC_LITERAL(4, 58, 16), // "valueChangedSlot"
QT_MOC_LITERAL(5, 75, 11), // "QtProperty*"
QT_MOC_LITERAL(6, 87, 15), // "modeChangedSlot"
QT_MOC_LITERAL(7, 103, 4) // "mode"

    },
    "netanim::AnimPropertyBroswer\0"
    "nodeIdSelectorSlot\0\0newIndex\0"
    "valueChangedSlot\0QtProperty*\0"
    "modeChangedSlot\0mode"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_netanim__AnimPropertyBroswer[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    1,   44,    2, 0x08 /* Private */,
       4,    2,   47,    2, 0x08 /* Private */,
       4,    2,   52,    2, 0x08 /* Private */,
       4,    2,   57,    2, 0x08 /* Private */,
       4,    2,   62,    2, 0x08 /* Private */,
       6,    1,   67,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, 0x80000000 | 5, QMetaType::QString,    2,    2,
    QMetaType::Void, 0x80000000 | 5, QMetaType::Double,    2,    2,
    QMetaType::Void, 0x80000000 | 5, QMetaType::QColor,    2,    2,
    QMetaType::Void, 0x80000000 | 5, QMetaType::Bool,    2,    2,
    QMetaType::Void, QMetaType::QString,    7,

       0        // eod
};

void netanim::AnimPropertyBroswer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        AnimPropertyBroswer *_t = static_cast<AnimPropertyBroswer *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->nodeIdSelectorSlot((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 1: _t->valueChangedSlot((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 2: _t->valueChangedSlot((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< double(*)>(_a[2]))); break;
        case 3: _t->valueChangedSlot((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< QColor(*)>(_a[2]))); break;
        case 4: _t->valueChangedSlot((*reinterpret_cast< QtProperty*(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 5: _t->modeChangedSlot((*reinterpret_cast< QString(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject netanim::AnimPropertyBroswer::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_netanim__AnimPropertyBroswer.data,
      qt_meta_data_netanim__AnimPropertyBroswer,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *netanim::AnimPropertyBroswer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *netanim::AnimPropertyBroswer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_netanim__AnimPropertyBroswer.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int netanim::AnimPropertyBroswer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
