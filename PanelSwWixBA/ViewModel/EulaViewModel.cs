//-------------------------------------------------------------------------------------------------
// <copyright file="EulaViewModel.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the _root directory of the distribution.
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
    public class EulaViewModel : ViewModelBase
    {
        public EulaViewModel(RootViewModel root)
            : base(root)
        {
        }

        #region Next - Button4

        private ICommand _nextCommand;
        public override ICommand /* NextCommand */ Button4Command
        {
            get
            {
                if (this._nextCommand == null)
                {
                    this._nextCommand = new RelayCommand(
                        param =>
                        {
                            _root.CurrentView = _root.InstallDirView;
                        }
                    );
                }

                return this._nextCommand;
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
                return "Next";
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
                            _root.CurrentView = _root.WelcomeView;
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


        public string LicenseText
        {
            get
            {
                string rtfPath = System.Reflection.Assembly.GetExecutingAssembly().Location;
                rtfPath = Path.GetDirectoryName(rtfPath);
                rtfPath = Path.Combine(rtfPath, "Eula.txt");

                if (!File.Exists(rtfPath))
                {
                    return "";
                }

                return File.ReadAllText(rtfPath);
            }
            set
            { }
        }
    }
}
