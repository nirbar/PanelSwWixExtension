//-------------------------------------------------------------------------------------------------
// <copyright file="WelcomeViewModel.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
//-------------------------------------------------------------------------------------------------

using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;
using PanelSW.WixBA.View;
using System.ComponentModel;
using System.DirectoryServices.ActiveDirectory;
using System.IO;
using System;
using System.Reflection;
using System.Windows.Input;
using System.Windows;

namespace PanelSW.WixBA
{
    public class WelcomeViewModel : ViewModelBase
    {
        private ICommand _installCommand;
        private ICommand _repairCommand;
        private ICommand _uninstallCommand;

        public WelcomeViewModel(RootViewModel root)
			: base( root)
        {
            this._root.PropertyChanged += new System.ComponentModel.PropertyChangedEventHandler(this.RootPropertyChanged);
        }

        public override string Title
        {
            get
            {
                return "Welcome";
            }
        }

        void RootPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            if ("State" == e.PropertyName)
            {
                base.OnPropertyChanged("Button2Visibility");
                base.OnPropertyChanged("Button3Visibility");
                base.OnPropertyChanged("Button4Visibility");
            }
        }
		
		#region Install - Button4

        public override ICommand /* InstallCommand */ Button4Command
        {
            get
            {
                if (this._installCommand == null)
                {
                    this._installCommand = new RelayCommand(
                        param =>
                        {
                            PanelSwWixBA.Model.PlannedAction = LaunchAction.Install;
                            _root.CurrentView = _root.EulaView;
                        },
                        param => this._root.State == InstallationState.DetectedAbsent);
                }

                return this._installCommand;
            }
        }

        public override Visibility /* InstallEnabled */ Button4Visibility
        {
            get 
			{
                return this._installCommand.CanExecute(this) ? Visibility.Visible : Visibility.Collapsed;
			}
        }

        public override object Button4Content
		{
			get
			{
				return "Install";
			}
		}
		
		#endregion
		
		#region Repair - Button3

        public override ICommand /* RepairCommand */ Button3Command
        {
            get
            {
                if (this._repairCommand == null)
                {
                    this._repairCommand = new RelayCommand(
                        param => 
                            {
                                PanelSwWixBA.Model.PlannedAction = LaunchAction.Repair;
                                _root.CurrentView = _root.EulaView;
                            },
                        param => this._root.State == InstallationState.DetectedPresent);
                }

                return this._repairCommand;
            }
        }

        public override Visibility /* RepairEnabled */ Button3Visibility
        {
            get 
			{
                return this._repairCommand.CanExecute(this) ? Visibility.Visible : Visibility.Collapsed;
			}
        }

        public override object Button3Content
		{
			get
			{
				return "Repair";
			}
		}
		
		#endregion

		#region Remove - Button2

        public override ICommand /* UninstallCommand */ Button2Command
        {
            get
            {
                if (this._uninstallCommand == null)
                {
                    this._uninstallCommand = new RelayCommand(
                        param => 
                            {
                                PanelSwWixBA.Plan(LaunchAction.Uninstall);
                                _root.CurrentView = (BaseView)_root.ProgressView;
                            },
                        param => this._root.State == InstallationState.DetectedPresent);
                }

                return this._uninstallCommand;
            }
        }

        public override Visibility Button2Visibility
        {
            get
            {
                return this._uninstallCommand.CanExecute(this) ? Visibility.Visible : Visibility.Collapsed;
            }
        }

        public override object Button2Content
		{
			get
			{
				return "Remove";
			}
        }

        #endregion
    }
}
