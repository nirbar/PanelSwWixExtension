//-------------------------------------------------------------------------------------------------
// <copyright file="Hresult.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// Utility class to work with HRESULTs
// </summary>
//-------------------------------------------------------------------------------------------------

namespace PanelSW.WixBA
{
    using System;

    /// <summary>
    /// Utility class to work with HRESULTs
    /// </summary>
    internal class Hresult
    {
        /// <summary>
        /// Determines if an HRESULT was a success code or not.
        /// </summary>
        /// <param name="status">HRESULT to verify.</param>
        /// <returns>True if the status is a success code.</returns>
        public static bool Succeeded(int status)
        {
            return status >= 0;
        }
    }
}
