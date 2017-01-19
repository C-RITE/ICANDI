/* Jungo Confidential. Copyright (c) 2008 Jungo Ltd.  http://www.jungo.com */

#ifndef _DIFX_INSTALL_H_
#define _DIFX_INSTALL_H_

BOOL difx_install_inf(const char *file_name_with_path, BOOL signed_install);
BOOL difx_preinstall_inf(const char *file_name_with_path, BOOL signed_install);
BOOL difx_uninstall_inf(const char *file_name_with_path,
    BOOL *driver_not_in_store);

#endif

