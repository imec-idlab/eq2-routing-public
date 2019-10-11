/****************************************************************************
** Meta object code from reading C++ file 'statsmode.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.9.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "statsmode.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'statsmode.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.9.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_netanim__NodeButton_t {
    QByteArrayData data[3];
    char stringdata0[39];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_netanim__NodeButton_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_netanim__NodeButton_t qt_meta_stringdata_netanim__NodeButton = {
    {
QT_MOC_LITERAL(0, 0, 19), // "netanim::NodeButton"
QT_MOC_LITERAL(1, 20, 17), // "buttonClickedSlot"
QT_MOC_LITERAL(2, 38, 0) // ""

    },
    "netanim::NodeButton\0buttonClickedSlot\0"
    ""
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_netanim__NodeButton[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   19,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void netanim::NodeButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        NodeButton *_t = static_cast<NodeButton *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->buttonClickedSlot(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject netanim::NodeButton::staticMetaObject = {
    { &QPushButton::staticMetaObject, qt_meta_stringdata_netanim__NodeButton.data,
      qt_meta_data_netanim__NodeButton,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *netanim::NodeButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *netanim::NodeButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_netanim__NodeButton.stringdata0))
        return static_cast<void*>(this);
    return QPushButton::qt_metacast(_clname);
}

int netanim::NodeButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QPushButton::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 1)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 1)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 1;
    }
    return _id;
}
struct qt_meta_stringdata_netanim__StatsMode_t {
    QByteArrayData data[17];
    char stringdata0[282];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_netanim__StatsMode_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_netanim__StatsMode_t qt_meta_stringdata_netanim__StatsMode = {
    {
QT_MOC_LITERAL(0, 0, 18), // "netanim::StatsMode"
QT_MOC_LITERAL(1, 19, 8), // "testSlot"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 29), // "clickRoutingTraceFileOpenSlot"
QT_MOC_LITERAL(4, 59, 18), // "selectAllNodesSlot"
QT_MOC_LITERAL(5, 78, 20), // "deselectAllNodesSlot"
QT_MOC_LITERAL(6, 99, 19), // "statTypeChangedSlot"
QT_MOC_LITERAL(7, 119, 5), // "index"
QT_MOC_LITERAL(8, 125, 18), // "updateTimelineSlot"
QT_MOC_LITERAL(9, 144, 5), // "value"
QT_MOC_LITERAL(10, 150, 12), // "fontSizeSlot"
QT_MOC_LITERAL(11, 163, 29), // "clickFlowMonTraceFileOpenSlot"
QT_MOC_LITERAL(12, 193, 23), // "allowedNodesChangedSlot"
QT_MOC_LITERAL(13, 217, 12), // "allowedNodes"
QT_MOC_LITERAL(14, 230, 23), // "counterIndexChangedSlot"
QT_MOC_LITERAL(15, 254, 13), // "counterString"
QT_MOC_LITERAL(16, 268, 13) // "showChartSlot"

    },
    "netanim::StatsMode\0testSlot\0\0"
    "clickRoutingTraceFileOpenSlot\0"
    "selectAllNodesSlot\0deselectAllNodesSlot\0"
    "statTypeChangedSlot\0index\0updateTimelineSlot\0"
    "value\0fontSizeSlot\0clickFlowMonTraceFileOpenSlot\0"
    "allowedNodesChangedSlot\0allowedNodes\0"
    "counterIndexChangedSlot\0counterString\0"
    "showChartSlot"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_netanim__StatsMode[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   69,    2, 0x0a /* Public */,
       3,    0,   70,    2, 0x08 /* Private */,
       4,    0,   71,    2, 0x08 /* Private */,
       5,    0,   72,    2, 0x08 /* Private */,
       6,    1,   73,    2, 0x08 /* Private */,
       8,    1,   76,    2, 0x08 /* Private */,
      10,    1,   79,    2, 0x08 /* Private */,
      11,    0,   82,    2, 0x08 /* Private */,
      12,    1,   83,    2, 0x08 /* Private */,
      14,    1,   86,    2, 0x08 /* Private */,
      16,    0,   89,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   13,
    QMetaType::Void, QMetaType::QString,   15,
    QMetaType::Void,

       0        // eod
};

void netanim::StatsMode::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        StatsMode *_t = static_cast<StatsMode *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->testSlot(); break;
        case 1: _t->clickRoutingTraceFileOpenSlot(); break;
        case 2: _t->selectAllNodesSlot(); break;
        case 3: _t->deselectAllNodesSlot(); break;
        case 4: _t->statTypeChangedSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->updateTimelineSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->fontSizeSlot((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 7: _t->clickFlowMonTraceFileOpenSlot(); break;
        case 8: _t->allowedNodesChangedSlot((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 9: _t->counterIndexChangedSlot((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 10: _t->showChartSlot(); break;
        default: ;
        }
    }
}

const QMetaObject netanim::StatsMode::staticMetaObject = {
    { &Mode::staticMetaObject, qt_meta_stringdata_netanim__StatsMode.data,
      qt_meta_data_netanim__StatsMode,  qt_static_metacall, nullptr, nullptr}
};


const QMetaObject *netanim::StatsMode::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *netanim::StatsMode::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_netanim__StatsMode.stringdata0))
        return static_cast<void*>(this);
    return Mode::qt_metacast(_clname);
}

int netanim::StatsMode::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = Mode::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
