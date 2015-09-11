//-------------------------------------------------------------------------------------------------
// <copyright file="FinishViewModel.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
//-------------------------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;
using System.Windows.Input;
using PanelSW.WixBA.View;

namespace PanelSW.WixBA
{
    public class FinishViewModel : ViewModelBase
    {
        public FinishViewModel(RootViewModel root)
			: base( root)
        {
        }

        public override string Title
        {
            get
            {
                switch (_root.State)
                {
                    case InstallationState.Failed:
                        return "There was an error with the configuration. Please refer to the log file at:\n" + PanelSwWixBA.Model.Engine.StringVariables["WixBundleLog"];

                    case InstallationState.Applied:
                        return "Configuration is complete. Thank You!";

                    default:
                        return "";
                }
            }
        }
    }
}
