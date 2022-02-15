/* log_impl.h
 *
 */

/*****************************************************************************
* Copyright (c) 2017 INSIDE Secure B.V. All Rights Reserved.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef INCLUDE_GUARD_LOG_IMPL_H
#define INCLUDE_GUARD_LOG_IMPL_H

#include "vault_driver.h"


#define Log_FormattedMessage  VAULT_LOGI

// backwards compatible implementation
#define Log_FormattedMessageINFO  VAULT_LOGI
#define Log_FormattedMessageWARN  VAULT_LOGI
#define Log_FormattedMessageCRIT  VAULT_LOGI


#endif /* Include Guard */


/* end of file log_impl.h */
