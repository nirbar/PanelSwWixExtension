//-------------------------------------------------------------------------------------------------
// <copyright file="PanelSwWixBA.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// The WiX toolset user experience.
// </summary>
//-------------------------------------------------------------------------------------------------

namespace PanelSW.WixBA
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.IO;
    using System.Net;
    using System.Text;
    using System.Windows.Input;
    using Threading = System.Windows.Threading;
    using WinForms = System.Windows.Forms;

    using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;

    /// <summary>
    /// The WiX toolset user experience.
    /// </summary>
    public class PanelSwWixBA : BootstrapperApplication
    {
        /// <summary>
        /// Gets the global model.
        /// </summary>
        static public Model Model { get; private set; }

        /// <summary>
        /// Gets the global view.
        /// </summary>
        static public RootView View { get; private set; }
        // TODO: We should refactor things so we dont have a global View.

        /// <summary>
        /// Gets the global dispatcher.
        /// </summary>
        static public Threading.Dispatcher Dispatcher { get; private set; }

        /// <summary>
        /// Launches the default web browser to the provided URI.
        /// </summary>
        /// <param name="uri">URI to open the web browser.</param>
        public static void LaunchUrl(string uri)
        {
            // Switch the wait cursor since shellexec can take a second or so.
            Cursor cursor = PanelSwWixBA.View.Cursor;
            PanelSwWixBA.View.Cursor = Cursors.Wait;

            try
            {
                Process process = new Process();
                process.StartInfo.FileName = uri;
                process.StartInfo.UseShellExecute = true;
                process.StartInfo.Verb = "open";

                process.Start();
            }
            finally
            {
                PanelSwWixBA.View.Cursor = cursor; // back to the original cursor.
            }
        }

        /// <summary>
        /// Starts planning the appropriate action.
        /// </summary>
        /// <param name="action">Action to plan.</param>
        public static void Plan(LaunchAction action)
        {
            PanelSwWixBA.Model.PlannedAction = action;
            PanelSwWixBA.Model.Engine.Plan(PanelSwWixBA.Model.PlannedAction);
        }

        public static void PlanLayout()
        {
            // Either default or set the layout directory
            if (String.IsNullOrEmpty(PanelSwWixBA.Model.Command.LayoutDirectory))
            {
                PanelSwWixBA.Model.LayoutDirectory = Directory.GetCurrentDirectory();

                // Ask the user for layout folder if one wasn't provided and we're in full UI mode
                if (PanelSwWixBA.Model.Command.Display == Display.Full)
                {
                    PanelSwWixBA.Dispatcher.Invoke((Action)delegate()
                    {
                        WinForms.FolderBrowserDialog browserDialog = new WinForms.FolderBrowserDialog();
                        browserDialog.RootFolder = Environment.SpecialFolder.MyComputer;

                        // Default to the current directory.
                        browserDialog.SelectedPath = PanelSwWixBA.Model.LayoutDirectory;
                        WinForms.DialogResult result = browserDialog.ShowDialog();

                        if (WinForms.DialogResult.OK == result)
                        {
                            PanelSwWixBA.Model.LayoutDirectory = browserDialog.SelectedPath;
                            PanelSwWixBA.Plan(PanelSwWixBA.Model.Command.Action);
                        }
                        else
                        {
                            PanelSwWixBA.View.Close();
                        }
                    }
                    );
                }
            }
            else
            {
                PanelSwWixBA.Model.LayoutDirectory = PanelSwWixBA.Model.Command.LayoutDirectory;
                PanelSwWixBA.Plan(PanelSwWixBA.Model.Command.Action);
            }
        }

        /// <summary>
        /// Thread entry point for WiX Toolset UX.
        /// </summary>
        protected override void Run()
        {
            this.Engine.Log(LogLevel.Verbose, "Running the WiX BA.");
            PanelSwWixBA.Model = new Model(this);
            PanelSwWixBA.Dispatcher = Threading.Dispatcher.CurrentDispatcher;
            RootViewModel viewModel = this.RootViewModel;

            // Kick off detect which will populate the view models.
            this.Engine.Detect();

            // Create a Window to show UI.
            if (PanelSwWixBA.Model.Command.Display == Display.Passive ||
                PanelSwWixBA.Model.Command.Display == Display.Full)
            {
                this.Engine.Log(LogLevel.Verbose, "Creating a UI.");
                PanelSwWixBA.View = new RootView(viewModel);
                PanelSwWixBA.View.Show();
            }

            Threading.Dispatcher.Run();

            this.Engine.Quit(PanelSwWixBA.Model.Result);
        }

        private RootViewModel _rootViewModel = null;
        protected virtual RootViewModel RootViewModel
        {
            get
            {
                if( _rootViewModel == null)
                {
                    _rootViewModel = new RootViewModel();
                }
                return _rootViewModel;
            }
        }
    }
}
