//-------------------------------------------------------------------------------------------------
// <copyright file="InstallDirViewModel.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the _root directory of the distribution.
// </copyright>
//-------------------------------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;
using System.Windows.Input;
using PanelSW.WixBA.View;
using System.Windows;
using System.Windows.Forms;

namespace PanelSW.WixBA
{
    /// <summary>
    /// Validate that the machine is part of a domain 
    /// </summary>
    public class InstallDirViewModel : ViewModelBase
    {
        public InstallDirViewModel(RootViewModel root)
			: base( root)
        {
        }

        public override string Title
        {
            get
            {
                return "Target Folder";
            }
        }

		
		#region Install - Button4

        private ICommand _installCommand;
        public override ICommand /* NextCommand */ Button4Command
        {
            get
            {
                if (this._installCommand == null)
                {
                    this._installCommand = new RelayCommand(
                        param =>
                        {
                            if (PanelSwWixBA.Model.ShowSqlWindows)
                            {
                                _root.CurrentView = _root.DbAccountView;
                            }
                            else
                            {
                                PanelSwWixBA.Plan(LaunchAction.Install);
                                _root.CurrentView = _root.ProgressView;
                            }
                        }
                    );
                }

                return this._installCommand;
            }
        }

        public override Visibility /* InstallEnabled */ Button4Visibility
        {
            get 
			{ 
				return Visibility.Visible;
			}
        }

		public override object Button4Content
		{
			get
			{
                if (PanelSwWixBA.Model.ShowSqlWindows)
                {
                    return "Next";
                }
                else
                {
                    return "Install";
                }
			}
		}
		
		#endregion

		
		#region Back - Button3

        private ICommand _backCommand;
        public override ICommand /* BackCommand */ Button3Command
        {
            get
            {
                if (this._backCommand == null)
                {
                    this._backCommand = new RelayCommand(
                        param =>
                        {
                            _root.CurrentView = _root.EulaView;
                        }
                    );
                }

                return this._backCommand;
            }
        }

        public override Visibility /* InstallEnabled */ Button3Visibility
        {
            get 
			{ 
				return Visibility.Visible;
			}
        }

        public override object Button3Content
		{
			get
			{
				return "Back";
			}
		}
		
		#endregion


        private ICommand _browseDirCommand = null;
        public ICommand BrowseDirCommand
        {
            get
            {
                if( _browseDirCommand==null)
                {
                    _browseDirCommand = new RelayCommand(
                        (a) =>
                        {
                            PanelSwWixBA.Dispatcher.Invoke((Action)delegate()
                            {
                                FolderBrowserDialog fbd = new FolderBrowserDialog();
                                fbd.ShowNewFolderButton = true;
                                fbd.SelectedPath = _root.InstallDirectory;
                                if (fbd.ShowDialog() == DialogResult.OK)
                                {
                                    _root.InstallDirectory = fbd.SelectedPath;
                                }
                            });
                        }
                        );
                }

                return _browseDirCommand;
            }
        }
    }
}