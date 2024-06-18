/*
    SPDX-FileCopyrightText: 2012, 2013 Martin Graesslin <mgraesslin@kde.org>
    SPDX-FileCopyrightText: 2015 David Edmudson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <xcb/composite.h>
#include <xcb/damage.h>
#include <xcb/randr.h>
#include <xcb/shm.h>
#include <xcb/xcb.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_event.h>
#include <X11/Xlib-xcb.h>

#include <QScopedPointer>
#include <QGuiApplication>
#include <QVector>

/** XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY 0
#define XEMBED_WINDOW_ACTIVATE 1
#define XEMBED_WINDOW_DEACTIVATE 2
#define XEMBED_REQUEST_FOCUS 3
#define XEMBED_FOCUS_IN 4
#define XEMBED_FOCUS_OUT 5
#define XEMBED_FOCUS_NEXT 6
#define XEMBED_FOCUS_PREV 7

namespace Xcb
{
typedef xcb_window_t WindowId;

template<typename T>
using ScopedCPointer = QScopedPointer<T, QScopedPointerPodDeleter>;

class Atom
{
public:
    explicit Atom(const QByteArray &name, bool onlyIfExists = false, xcb_connection_t *c = XGetXCBConnection(qApp->nativeInterface<QNativeInterface::QX11Application>()->display()))
        : m_connection(c)
        , m_retrieved(false)
        , m_cookie(xcb_intern_atom_unchecked(m_connection, onlyIfExists, name.length(), name.constData()))
        , m_atom(XCB_ATOM_NONE)
        , m_name(name)
    {
    }
    Atom() = delete;
    Atom(const Atom &) = delete;

    ~Atom()
    {
        if (!m_retrieved && m_cookie.sequence) {
            xcb_discard_reply(m_connection, m_cookie.sequence);
        }
    }

    operator xcb_atom_t() const
    {
        (const_cast<Atom *>(this))->getReply();
        return m_atom;
    }
    bool isValid()
    {
        getReply();
        return m_atom != XCB_ATOM_NONE;
    }
    bool isValid() const
    {
        (const_cast<Atom *>(this))->getReply();
        return m_atom != XCB_ATOM_NONE;
    }

    inline const QByteArray &name() const
    {
        return m_name;
    }

private:
    void getReply()
    {
        if (m_retrieved || !m_cookie.sequence) {
            return;
        }
        ScopedCPointer<xcb_intern_atom_reply_t> reply(xcb_intern_atom_reply(m_connection, m_cookie, nullptr));
        if (!reply.isNull()) {
            m_atom = reply->atom;
        }
        m_retrieved = true;
    }
    xcb_connection_t *m_connection;
    bool m_retrieved;
    xcb_intern_atom_cookie_t m_cookie;
    xcb_atom_t m_atom;
    QByteArray m_name;
};

class Atoms
{
public:
    Atoms()
    {
        xcb_connection_t *connection = QNativeInterface::QX11Application::connection();
        
        xembedAtom = internAtom(connection, "_XEMBED");
        selectionAtom = internAtom(connection, "_NET_SYSTEM_TRAY_S" + std::to_string(QApplication::primaryScreen()->geometry().x()));
        opcodeAtom = internAtom(connection, "_NET_SYSTEM_TRAY_OPCODE");
        messageData = internAtom(connection, "_NET_SYSTEM_TRAY_MESSAGE_DATA");
        visualAtom = internAtom(connection, "_NET_SYSTEM_TRAY_VISUAL");
    }

    Atom internAtom(xcb_connection_t *connection, const std::string &atomName)
    {
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 0, atomName.length(), atomName.c_str());
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
        Atom atom = reply ? reply->atom : XCB_ATOM_NONE;
        free(reply);
        return atom;
    }

    Atom xembedAtom;
    Atom selectionAtom;
    Atom opcodeAtom;
    Atom messageData;
    Atom visualAtom;
};

extern Atoms *atoms;

}// namespace Xcb
