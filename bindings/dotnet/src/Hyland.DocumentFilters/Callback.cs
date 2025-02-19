//===========================================================================
// (c) 2020 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.IO;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Represents the callback for opening a document.
    /// </summary>
    public class OpenCallback
    {
        /// <summary>
        /// Gets or sets the action to be performed.
        /// </summary>
        public int Action { get; internal set; }

        /// <summary>
        /// Gets or sets the heartbeat action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Heartbeat Heartbeat { get; internal set; }

        /// <summary>
        /// Gets or sets the password action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Password Password { get; internal set; }

        /// <summary>
        /// Gets or sets the localize action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Localize Localize { get; internal set; }

        /// <summary>
        /// Gets or sets the log action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Log_Level LogLevel { get; internal set; }

        /// <summary>
        /// Gets or sets the log message action payload.
        /// </summary>
        public IGR_Open_Callback_Action_Log_Message LogMessage { get; internal set; }
    }
}

