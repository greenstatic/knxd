/*
    EIBD eib bus access and management daemon
    Copyright (C) 2005-2011 Martin Koegler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "emi2.h"

#include "emi.h"

EMI2Driver::EMI2Driver (LowLevelIface* c, IniSectionPtr& s, LowLevelDriver *i) : EMI_Common(c,s,i)
{
  t->setAuxName("EMI2");
  sendLocal_done.set<EMI2Driver,&EMI2Driver::sendLocal_done_cb>(this);
  reset_timer.set<EMI2Driver,&EMI2Driver::reset_timer_cb>(this);
}

void
EMI2Driver::sendLocal_done_cb(bool success)
{
  if (!success || sendLocal_done_next == N_bad)
    {
      stop(true);
      LowLevelFilter::stop(true);
    }
  else if (sendLocal_done_next == N_down)
    LowLevelFilter::stop(false);
  else if (sendLocal_done_next == N_up)
    LowLevelFilter::started();
  else if (sendLocal_done_next == N_open)
    {
      sendLocal_done_next = N_up;
      const uint8_t t2[] = { 0xa9, 0x00, 0x18, 0x34, 0x56, 0x78, 0x0a };
      send_Local (CArray (t2, sizeof (t2)),1);
    }
  else if (sendLocal_done_next == N_enter)
    {
      sendLocal_done_next = N_up;
      const uint8_t t2[] = { 0xa9, 0x90, 0x18, 0x34, 0x45, 0x67, 0x8a };
      send_Local (CArray (t2, sizeof (t2)),1);
    }

}

void
EMI2Driver::cmdEnterMonitor ()
{
  sendLocal_done_next = N_enter;
  const uint8_t t1[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  send_Local (CArray (t1, sizeof (t1)),1);
}

void
EMI2Driver::cmdLeaveMonitor ()
{
  if (wait_confirm_low)
    {
      sendLocal_done_next = N_want_leave;
      return;
    }
  sendLocal_done_next = N_down;
  uint8_t t[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  send_Local (CArray (t, sizeof (t)),1);
}

void
EMI2Driver::cmdOpen ()
{
  sendLocal_done_next = N_open;
  const uint8_t t1[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  send_Local (CArray (t1, sizeof (t1)),1);
}

void
EMI2Driver::cmdClose ()
{
  if (wait_confirm_low)
    {
      sendLocal_done_next = N_want_close;
      return;
    }
  sendLocal_done_next = N_down;
  uint8_t t[] = { 0xa9, 0x1E, 0x12, 0x34, 0x56, 0x78, 0x9a };
  send_Local (CArray (t, sizeof (t)),1);
}

void EMI2Driver::started()
{
  reset_ack_wait = true;
  reset_timer.start(0.5, 0);
  sendReset();
}

void EMI2Driver::reset_timer_cb(ev::timer&, int)
{
  ERRORPRINTF(t, E_ERROR | 57, "reset timed out");
  stop(true);
}

void EMI2Driver::do_send_Next()
{
  if (reset_ack_wait)
    {
      reset_ack_wait = false;
      reset_timer.stop();
      EMI_Common::started();
    }
  else if (sendLocal_done_next == N_want_close)
    {
      wait_confirm_low = false;
      cmdClose();
    }
  else if (sendLocal_done_next == N_want_leave)
    {
      wait_confirm_low = false;
      cmdLeaveMonitor();
    }
  else
    EMI_Common::do_send_Next();
}

const uint8_t *
EMI2Driver::getIndTypes() const
{
  static const uint8_t indTypes[] = { 0x2E, 0x29, 0x2B };
  return indTypes;
}

