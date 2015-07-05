//-------------------------------------------------------------------------------------------------
// <copyright file="ProgressViewModel.cs" company="Panel-SW.com">
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
using System.Windows;
using PanelSW.WixBA.View;

namespace PanelSW.WixBA
{
    public class ProgressViewModel : ViewModelBase
    {
        private RootViewModel root;
        private Dictionary<string, int> executingPackageOrderIndex;
        private ICommand cancelCommand;

        private int progressPhases;
        private int progress;
        private int cacheProgress;
        private int executeProgress;
        private string message;

        public ProgressViewModel(RootViewModel root)
            : base( root)
        {
            this.root = root;
            this.executingPackageOrderIndex = new Dictionary<string, int>();

            this.root.PropertyChanged += this.RootPropertyChanged;

            PanelSwWixBA.Model.Bootstrapper.ExecuteMsiMessage += this.ExecuteMsiMessage;
            PanelSwWixBA.Model.Bootstrapper.ExecuteProgress += this.ApplyExecuteProgress;
            PanelSwWixBA.Model.Bootstrapper.ApplyComplete += this.ApplyComplete;
            PanelSwWixBA.Model.Bootstrapper.PlanBegin += this.PlanBegin;
            PanelSwWixBA.Model.Bootstrapper.PlanPackageComplete += this.PlanPackageComplete;
            PanelSwWixBA.Model.Bootstrapper.Progress += this.ApplyProgress;
            PanelSwWixBA.Model.Bootstrapper.CacheAcquireProgress += this.CacheAcquireProgress;
            PanelSwWixBA.Model.Bootstrapper.CacheComplete += this.CacheComplete;
        }

        public bool ProgressEnabled
        {
            get { return this.root.State == InstallationState.Applying; }
        }

        public int Progress
        {
            get
            {
                return this.progress;
            }

            set
            {
                if (this.progress != value)
                {
                    this.progress = value;
                    base.OnPropertyChanged("Progress");
                }
            }
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

        void RootPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if ("State" == e.PropertyName)
            {
                base.OnPropertyChanged("ProgressEnabled");
                base.OnPropertyChanged("Button1Visibility");
            }
        }

        private void ApplyComplete(object sender, ApplyCompleteEventArgs e)
        {
            if( Hresult.Succeeded( e.Status))
            {
                root.State = InstallationState.Applied;
            }
            else
            {
                root.State = InstallationState.Failed;
            }
            StartFinishView();
        }

        private void PlanBegin(object sender, PlanBeginEventArgs e)
        {
            lock (this)
            {
                this.progressPhases = (LaunchAction.Layout == PanelSwWixBA.Model.PlannedAction) ? 1 : 2;
                this.executingPackageOrderIndex.Clear();
            }
        }

        private void PlanPackageComplete(object sender, PlanPackageCompleteEventArgs e)
        {
            if (ActionState.None != e.Execute)
            {
                lock (this)
                {
                    Debug.Assert(!this.executingPackageOrderIndex.ContainsKey(e.PackageId));
                    this.executingPackageOrderIndex.Add(e.PackageId, this.executingPackageOrderIndex.Count);
                }
            }
        }

        private void ExecuteMsiMessage(object sender, ExecuteMsiMessageEventArgs e)
        {
            lock (this)
            {
                if (e.MessageType == InstallMessage.ActionStart)
                {
                    this.Message = e.Message;
                }

                e.Result = this.root.Canceled ? Result.Cancel : Result.Ok;
            }
        }

        private void ApplyProgress(object sender, ProgressEventArgs e)
        {
            lock (this)
            {
                e.Result = this.root.Canceled ? Result.Cancel : Result.Ok;
            }
        }

        private void CacheAcquireProgress(object sender, CacheAcquireProgressEventArgs e)
        {
            lock (this)
            {
                this.cacheProgress = e.OverallPercentage;
                this.Progress = (this.cacheProgress + this.executeProgress) / this.progressPhases;
                e.Result = this.root.Canceled ? Result.Cancel : Result.Ok;
            }
        }

        private void CacheComplete(object sender, CacheCompleteEventArgs e)
        {
            lock (this)
            {
                this.cacheProgress = 100;
                this.Progress = (this.cacheProgress + this.executeProgress) / this.progressPhases;
            }
        }

        private void ApplyExecuteProgress(object sender, ExecuteProgressEventArgs e)
        {
            lock (this)
            {

                this.executeProgress = e.OverallPercentage;
                this.Progress = (this.cacheProgress + this.executeProgress) / 2; // always two phases if we hit execution.

                if (PanelSwWixBA.Model.Command.Display == Display.Embedded)
                {
                    PanelSwWixBA.Model.Engine.SendEmbeddedProgress(e.ProgressPercentage, this.Progress);
                }

                e.Result = this.root.Canceled ? Result.Cancel : Result.Ok;
            }
        }

        public override ICommand /* CancelCommand */ Button1Command
        {
            get
            {
                if (this.cancelCommand == null)
                {
                    this.cancelCommand = new RelayCommand(param =>
                    {
                        lock (this)
                        {
                            MessageBoxResult res = MessageBox.Show(PanelSwWixBA.View, "Are you sure you want to cancel?", "Panel-SW Installer", MessageBoxButton.YesNo, MessageBoxImage.Error);
                            if( res == MessageBoxResult.Yes)
                            {
                                this.root.Canceled = true;
                                StartFinishView();
                            }
                        }
                    },
                    param => this.root.State == InstallationState.Applying);
                }

                return this.cancelCommand;
            }
        }

        public override Visibility Button1Visibility
        {
            get 
			{
                return this.Button1Command.CanExecute(this) ? Visibility.Visible : Visibility.Hidden;
			}
        }

        private void StartFinishView()
        {
            PanelSwWixBA.Dispatcher.Invoke(
                (Action)delegate()
                {
                    root.CurrentView = root.FinishView;
                }
            );
        }
    }
}
