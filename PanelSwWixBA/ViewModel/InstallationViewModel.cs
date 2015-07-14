//-------------------------------------------------------------------------------------------------
// <copyright file="InstallationViewModel.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// The model of the installation view.
// </summary>
//-------------------------------------------------------------------------------------------------

namespace PanelSW.WixBA
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Reflection;
    using System.Windows;
    using System.Windows.Input;
    using IO = System.IO;
    using Microsoft.Tools.WindowsInstallerXml;
    using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;

    /// <summary>
    /// The states of the installation view model.
    /// </summary>
    public enum InstallationState
    {
        Initializing,
        DetectedAbsent,
        DetectedPresent,
        DetectedNewer,
        Applying,
        Applied,
        Failed,
    }

    /// <summary>
    /// The model of the installation view in PanelSwWixBA.
    /// </summary>
    public class InstallationViewModel : ViewModelBase
    {
        private RootViewModel root;

        private Dictionary<string, int> downloadRetries;
        private bool downgrade;

        private ICommand launchHomePageCommand;

        private string message;
        private DateTime cachePackageStart;
        private DateTime executePackageStart;

        /// <summary>
        /// Creates a new model of the installation view.
        /// </summary>
        public InstallationViewModel(RootViewModel root)
            : base(root)
        {
            this.root = root;
            this.downloadRetries = new Dictionary<string, int>();

            this.root.PropertyChanged += new System.ComponentModel.PropertyChangedEventHandler(this.RootPropertyChanged);

            PanelSwWixBA.Model.Bootstrapper.DetectBegin += this.DetectBegin;
            PanelSwWixBA.Model.Bootstrapper.DetectRelatedBundle += this.DetectedRelatedBundle;
            PanelSwWixBA.Model.Bootstrapper.DetectComplete += this.DetectComplete;
            PanelSwWixBA.Model.Bootstrapper.PlanPackageBegin += this.PlanPackageBegin;
            PanelSwWixBA.Model.Bootstrapper.PlanComplete += this.PlanComplete;
            PanelSwWixBA.Model.Bootstrapper.ApplyBegin += this.ApplyBegin;
            PanelSwWixBA.Model.Bootstrapper.CacheAcquireBegin += this.CacheAcquireBegin;
            PanelSwWixBA.Model.Bootstrapper.CacheAcquireComplete += this.CacheAcquireComplete;
            PanelSwWixBA.Model.Bootstrapper.ExecutePackageBegin += this.ExecutePackageBegin;
            PanelSwWixBA.Model.Bootstrapper.ExecutePackageComplete += this.ExecutePackageComplete;
            PanelSwWixBA.Model.Bootstrapper.Error += this.ExecuteError;
            PanelSwWixBA.Model.Bootstrapper.ResolveSource += this.ResolveSource;
            PanelSwWixBA.Model.Bootstrapper.ApplyComplete += this.ApplyComplete;
        }

        void RootPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if ("State" == e.PropertyName)
            {
                base.OnPropertyChanged("Title");
            }
        }

        public String WixBundleManufacturer
        {
            get
            {
                return PanelSwWixBA.Model.WixBundleManufacturer;
            }
        }

        public String WixBundleName
        {
            get
            {
                return PanelSwWixBA.Model.WixBundleName;
            }
        }

        /// <summary>
        /// Gets the title for the application.
        /// </summary>
        public string WixBundleVersion 
        {
            get { return String.Concat("v", PanelSwWixBA.Model.WixBundleVersion.ToString()); }
        }

        public string Message
        {
            get
            {
                return this.message;
            }

            set
            {
                if (this.message != value)
                {
                    this.message = value;
                    base.OnPropertyChanged("Message");
                }
            }
        }

        /// <summary>
        /// Gets and sets whether the view model considers this install to be a downgrade.
        /// </summary>
        public bool Downgrade
        {
            get
            {
                return this.downgrade;
            }

            set
            {
                if (this.downgrade != value)
                {
                    this.downgrade = value;
                    base.OnPropertyChanged("Downgrade");
                }
            }
        }

        public ICommand LaunchHomePageCommand
        {
            get
            {
                if (this.launchHomePageCommand == null)
                {
                    this.launchHomePageCommand = new RelayCommand(
                        param => PanelSwWixBA.LaunchUrl(PanelSwWixBA.Model.Engine.StringVariables["WixBundleAboutUrl"]),
                        param => PanelSwWixBA.Model.Engine.StringVariables.Contains("WixBundleAboutUrl"));
                }

                return this.launchHomePageCommand;
            }
        }

        public override string Title
        {
            get
            {
                switch (this.root.State)
                {
                    case InstallationState.Initializing:
                        return "Initializing...";

                    case InstallationState.DetectedPresent:
                        return "Installed";

                    case InstallationState.DetectedNewer:
                        return "Newer version installed";

                    case InstallationState.DetectedAbsent:
                        return "Not installed";

                    case InstallationState.Applying:
                        switch (PanelSwWixBA.Model.PlannedAction)
                        {
                            case LaunchAction.Install:
                                return "Installing...";

                            case LaunchAction.Repair:
                                return "Repairing...";

                            case LaunchAction.Uninstall:
                                return "Uninstalling...";

                            case LaunchAction.UpdateReplace:
                            case LaunchAction.UpdateReplaceEmbedded:
                                return "Updating...";

                            default:
                                return "Unexpected action state";
                        }

                    case InstallationState.Applied:
                        switch (PanelSwWixBA.Model.PlannedAction)
                        {
                            case LaunchAction.Install:
                                return "Successfully installed";

                            case LaunchAction.Repair:
                                return "Successfully repaired";

                            case LaunchAction.Uninstall:
                                return "Successfully uninstalled";

                            case LaunchAction.UpdateReplace:
                            case LaunchAction.UpdateReplaceEmbedded:
                                return "Successfully updated";

                            default:
                                return "Unexpected action state";
                        }

                    case InstallationState.Failed:
                        if (this.root.Canceled)
                        {
                            return "Canceled";
                        }
                        else if (LaunchAction.Unknown != PanelSwWixBA.Model.PlannedAction)
                        {
                            switch (PanelSwWixBA.Model.PlannedAction)
                            {
                                case LaunchAction.Install:
                                    return "Failed to install";

                                case LaunchAction.Repair:
                                    return "Failed to repair";

                                case LaunchAction.Uninstall:
                                    return "Failed to uninstall";

                                case LaunchAction.UpdateReplace:
                                case LaunchAction.UpdateReplaceEmbedded:
                                    return "Failed to update";

                                default:
                                    return "Unexpected action state";
                            }
                        }
                        else
                        {
                            return "Unexpected failure";
                        }

                    default:
                        return "Unknown view model state";
                }
            }
        }


        private void DetectBegin(object sender, DetectBeginEventArgs e)
        {
            this.root.State = e.Installed ? InstallationState.DetectedPresent : InstallationState.DetectedAbsent;
            PanelSwWixBA.Model.PlannedAction = LaunchAction.Unknown;
        }

        private void DetectedRelatedBundle(object sender, DetectRelatedBundleEventArgs e)
        {
            if (e.Operation == RelatedOperation.Downgrade)
            {
                this.Downgrade = true;
            }
        }

        private void DetectComplete(object sender, DetectCompleteEventArgs e)
        {
            // Parse the command line string before any planning.
            this.ParseCommandLine();

            if (LaunchAction.Uninstall == PanelSwWixBA.Model.Command.Action)
            {
                PanelSwWixBA.Model.Engine.Log(LogLevel.Verbose, "Invoking automatic plan for uninstall");
                PanelSwWixBA.Plan(LaunchAction.Uninstall);
            }
            else if (Hresult.Succeeded(e.Status))
            {
                // block if CLR v2 isn't available; sorry, it's needed for the MSBuild tasks
                if (PanelSwWixBA.Model.Engine.EvaluateCondition("NETFRAMEWORK35_SP_LEVEL < 1"))
                {
                    string message = "WiX Toolset requires the .NET Framework 3.5.1 Windows feature to be enabled.";
                    PanelSwWixBA.Model.Engine.Log(LogLevel.Verbose, message);

                    if (Display.Full == PanelSwWixBA.Model.Command.Display)
                    {
                        PanelSwWixBA.Dispatcher.Invoke((Action)delegate()
                            {
                                MessageBox.Show(message, "WiX Toolset", MessageBoxButton.OK, MessageBoxImage.Error);
                                if (null != PanelSwWixBA.View)
                                {
                                    PanelSwWixBA.View.Close();
                                }
                            }
                        );
                    }

                    this.root.State = InstallationState.Failed;
                    return;
                }

                if (this.Downgrade)
                {
                    // TODO: What behavior do we want for downgrade?
                    this.root.State = InstallationState.DetectedNewer;
                }

                if (LaunchAction.Layout == PanelSwWixBA.Model.Command.Action)
                {
                    PanelSwWixBA.PlanLayout();
                }
                else if (PanelSwWixBA.Model.Command.Display != Display.Full)
                {
                    // If we're not waiting for the user to click install, dispatch plan with the default action.
                    PanelSwWixBA.Model.Engine.Log(LogLevel.Verbose, "Invoking automatic plan for non-interactive mode.");
                    PanelSwWixBA.Plan(PanelSwWixBA.Model.Command.Action);
                }
            }
            else
            {
                this.root.State = InstallationState.Failed;
            }
        }

        private void PlanPackageBegin(object sender, PlanPackageBeginEventArgs e)
        {
            if (PanelSwWixBA.Model.Engine.StringVariables.Contains("MbaNetfxPackageId") && e.PackageId.Equals(PanelSwWixBA.Model.Engine.StringVariables["MbaNetfxPackageId"], StringComparison.Ordinal))
            {
                e.State = RequestState.None;
            }
        }

        private void PlanComplete(object sender, PlanCompleteEventArgs e)
        {
            if (Hresult.Succeeded(e.Status))
            {
                this.root.PreApplyState = this.root.State;
                this.root.State = InstallationState.Applying;
                PanelSwWixBA.Model.Engine.Apply(this.root.ViewWindowHandle);
            }
            else
            {
                this.root.State = InstallationState.Failed;
            }
        }

        private void ApplyBegin(object sender, ApplyBeginEventArgs e)
        {
            this.downloadRetries.Clear();
        }

        private void CacheAcquireBegin(object sender, CacheAcquireBeginEventArgs e)
        {
            this.cachePackageStart = DateTime.Now;
        }

        private void CacheAcquireComplete(object sender, CacheAcquireCompleteEventArgs e)
        {
            this.AddPackageTelemetry("Cache", e.PackageOrContainerId ?? String.Empty, DateTime.Now.Subtract(this.cachePackageStart).TotalMilliseconds, e.Status);
        }

        private void ExecutePackageBegin(object sender, ExecutePackageBeginEventArgs e)
        {
            this.executePackageStart = e.ShouldExecute ? DateTime.Now : DateTime.MinValue;
        }

        private void ExecutePackageComplete(object sender, ExecutePackageCompleteEventArgs e)
        {
            if (DateTime.MinValue < this.executePackageStart)
            {
                this.AddPackageTelemetry("Execute", e.PackageId ?? String.Empty, DateTime.Now.Subtract(this.executePackageStart).TotalMilliseconds, e.Status);
                this.executePackageStart = DateTime.MinValue;
            }
        }

        private void ExecuteError(object sender, ErrorEventArgs e)
        {
            lock (this)
            {
                if (!this.root.Canceled)
                {
                    // If the error is a cancel coming from the engine during apply we want to go back to the preapply state.
                    if (InstallationState.Applying == this.root.State && (int)Error.UserCancelled == e.ErrorCode)
                    {
                        this.root.State = this.root.PreApplyState;
                    }
                    else
                    {
                        this.Message = e.ErrorMessage;

                        if (Display.Full == PanelSwWixBA.Model.Command.Display)
                        {
                            // On HTTP authentication errors, have the engine try to do authentication for us.
                            if (ErrorType.HttpServerAuthentication == e.ErrorType || ErrorType.HttpProxyAuthentication == e.ErrorType)
                            {
                                e.Result = Result.TryAgain;
                            }
                            else // show an error dialog.
                            {
                                MessageBoxButton msgbox = MessageBoxButton.OK;
                                switch (e.UIHint & 0xF)
                                {
                                    case 0:
                                        msgbox = MessageBoxButton.OK;
                                        break;
                                    case 1:
                                        msgbox = MessageBoxButton.OKCancel;
                                        break;
                                    // There is no 2! That would have been MB_ABORTRETRYIGNORE.
                                    case 3:
                                        msgbox = MessageBoxButton.YesNoCancel;
                                        break;
                                    case 4:
                                        msgbox = MessageBoxButton.YesNo;
                                        break;
                                    // default: stay with MBOK since an exact match is not available.
                                }

                                MessageBoxResult result = MessageBoxResult.None;
                                PanelSwWixBA.View.Dispatcher.Invoke((Action)delegate()
                                    {
                                        result = MessageBox.Show(PanelSwWixBA.View, e.ErrorMessage, "WiX Toolset", msgbox, MessageBoxImage.Error);
                                    }
                                    );

                                // If there was a match from the UI hint to the msgbox value, use the result from the
                                // message box. Otherwise, we'll ignore it and return the default to Burn.
                                if ((e.UIHint & 0xF) == (int)msgbox)
                                {
                                    e.Result = (Result)result;
                                }
                            }
                        }
                    }
                }
                else // canceled, so always return cancel.
                {
                    e.Result = Result.Cancel;
                }
            }
        }

        private void ResolveSource(object sender, ResolveSourceEventArgs e)
        {
            int retries = 0;

            this.downloadRetries.TryGetValue(e.PackageOrContainerId, out retries);
            this.downloadRetries[e.PackageOrContainerId] = retries + 1;

            e.Result = retries < 3 && !String.IsNullOrEmpty(e.DownloadSource) ? Result.Download : Result.Ok;
        }

        private void ApplyComplete(object sender, ApplyCompleteEventArgs e)
        {
            PanelSwWixBA.Model.Result = e.Status; // remember the final result of the apply.

            // If we're not in Full UI mode, we need to alert the dispatcher to stop and close the window for passive.
            if ( Microsoft.Tools.WindowsInstallerXml.Bootstrapper.Display.Full != PanelSwWixBA.Model.Command.Display)
            {
                // If its passive, send a message to the window to close.
                if (Microsoft.Tools.WindowsInstallerXml.Bootstrapper.Display.Passive == PanelSwWixBA.Model.Command.Display)
                {
                    PanelSwWixBA.Model.Engine.Log(LogLevel.Verbose, "Automatically closing the window for non-interactive install");
                    PanelSwWixBA.Dispatcher.BeginInvoke((Action)delegate()
                    {
                        PanelSwWixBA.View.Close();
                    }
                    );
                }
                else
                {
                    PanelSwWixBA.Dispatcher.InvokeShutdown();
                }
            }
            else if (Hresult.Succeeded(e.Status) && LaunchAction.UpdateReplace == PanelSwWixBA.Model.PlannedAction) // if we successfully applied an update close the window since the new Bundle should be running now.
            {
                PanelSwWixBA.Model.Engine.Log(LogLevel.Verbose, "Automatically closing the window since update successful.");
                PanelSwWixBA.Dispatcher.BeginInvoke((Action)delegate()
                {
                    PanelSwWixBA.View.Close();
                }
                );
            }

            // Set the state to applied or failed unless the state has already been set back to the preapply state
            // which means we need to show the UI as it was before the apply started.
            if (this.root.State != this.root.PreApplyState)
            {
                this.root.State = Hresult.Succeeded(e.Status) ? InstallationState.Applied : InstallationState.Failed;
            }
        }

        private void ParseCommandLine()
        {
            // Get array of arguments based on the system parsing algorithm.
            string[] args = PanelSwWixBA.Model.Command.GetCommandLineArgs();
            for (int i = 0; i < args.Length; ++i)
            {
                if (args[i].StartsWith("InstallFolder=", StringComparison.InvariantCultureIgnoreCase))
                {
                    // Allow relative directory paths. Also validates.
                    string[] param = args[i].Split(new char[] {'='}, 2);
                    this.root.InstallDirectory = IO.Path.Combine(Environment.CurrentDirectory, param[1]);
                }
            }
        }

        private void AddPackageTelemetry(string prefix, string id, double time, int result)
        {
            lock (this)
            {
                string key = String.Format("{0}Time_{1}", prefix, id);
                string value = time.ToString();
                PanelSwWixBA.Model.Telemetry.Add(new KeyValuePair<string, string>(key, value));

                key = String.Format("{0}Result_{1}", prefix, id);
                value = String.Concat("0x", result.ToString("x"));
                PanelSwWixBA.Model.Telemetry.Add(new KeyValuePair<string, string>(key, value));
            }
        }
    }
}
