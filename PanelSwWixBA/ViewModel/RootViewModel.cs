//-------------------------------------------------------------------------------------------------
// <copyright file="RootViewModel.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// The model of the view for the PanelSwWixBA.
// </summary>
//-------------------------------------------------------------------------------------------------

namespace PanelSW.WixBA
{
    using System;
    using System.Diagnostics;
    using System.Reflection;
    using System.Windows;
    using System.Windows.Input;
    using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;
    using PanelSW.WixBA.View;

    /// <summary>
    /// The errors returned from the engine
    /// </summary>
    public enum Error
    {
        UserCancelled = 1223,
    }

    /// <summary>
    /// The model of the root view in PanelSwWixBA.
    /// </summary>
    public class RootViewModel : PropertyNotifyBase
    {
        private ICommand closeCommand;

        private bool canceled;
        private InstallationState state;

        /// <summary>
        /// Creates a new model of the root view.
        /// </summary>
        public RootViewModel()
        {
            this.InstallationViewModel = new InstallationViewModel(this);
            if ((PanelSwWixBA.Model.Command.Display == Display.Passive)
                || (PanelSwWixBA.Model.Command.Action == LaunchAction.Uninstall))
            {
                this.CurrentView = ProgressView;
            }
            else
            {
                this.CurrentView = WelcomeView;
            }
        }

        public InstallationViewModel InstallationViewModel { get; private set; }
        public IntPtr ViewWindowHandle { get; set; }

        private string _title = null;
        public virtual string Title
        {
            get
            {
                return _title ?? PanelSwWixBA.Model.WixBundleName;
            }
            set
            {
                _title = value;
                OnPropertyChanged("Title");
            }
        }



        private BaseView _currentView;
        public BaseView CurrentView
        {
            get
            {
                return _currentView;
            }
            set
            {
                _currentView = value;
                OnPropertyChanged("CurrentView");
            }
        }

        public ICommand CloseCommand
        {
            get
            {
                if (this.closeCommand == null)
                {
                    this.closeCommand = new RelayCommand(param => 
                        {
                            bool shouldClose = false;

                            PanelSwWixBA.Dispatcher.Invoke(
                                (Action)delegate()
                                {
                                    System.Windows.Forms.DialogResult res= System.Windows.Forms.MessageBox.Show(
                                        "Are you sure you want to cancel the configuration?"
                                        , "Cancel?"
                                        , System.Windows.Forms.MessageBoxButtons.YesNo
                                        , System.Windows.Forms.MessageBoxIcon.Question
                                        );

                                    shouldClose = (res == System.Windows.Forms.DialogResult.Yes);
                                }
                                );

                            if (shouldClose)
                            {
                                PanelSwWixBA.View.Close();
                            }
                        }
                        );
                }

                return this.closeCommand;
            }
        }

        public bool Canceled
        {
            get
            {
                return this.canceled;
            }

            set
            {
                if (this.canceled != value)
                {
                    this.canceled = value;
                    base.OnPropertyChanged("Canceled");
                }
            }
        }

        /// <summary>
        /// Gets and sets the state of the view's model.
        /// </summary>
        public InstallationState State
        {
            get
            {
                return this.state;
            }

            set
            {
                if (this.state != value)
                {
                    this.state = value;

                    // Notify all the properties derived from the state that the state changed.
                    base.OnPropertyChanged("State");
                }
            }
        }

        /// <summary>
        /// Gets and sets the state of the view's model before apply begins in order to return to that state if cancel or rollback occurs.
        /// </summary>
        public InstallationState PreApplyState { get; set; }

        /// <summary>
        /// Gets and sets the path where the bundle is currently installed or will be installed.
        /// </summary>
        public string InstallDirectory
        {
            get
            {
                return PanelSwWixBA.Model.InstallDirectory;
            }

            set
            {
                if (PanelSwWixBA.Model.InstallDirectory != value)
                {
                    PanelSwWixBA.Model.InstallDirectory = value;
                    base.OnPropertyChanged("InstallDirectory");
                }
            }
        }

        #region Welcome

        private WelcomeView _welcomeView = null;
        public virtual WelcomeView WelcomeView
        {
            get
            {
                if (_welcomeView == null)
                {
                    _welcomeView = new WelcomeView(this);
                }
                return _welcomeView;
            }
        }

        private WelcomeViewModel _welcomeViewModel = null;
        public virtual WelcomeViewModel WelcomeViewModel
        {
            get
            {
                if (_welcomeViewModel == null)
                {
                    _welcomeViewModel = new WelcomeViewModel(this);
                }

                return _welcomeViewModel;
            }
        }

        #endregion

        #region Eula

        private EulaView _eulaView = null;
        public virtual EulaView EulaView
        {
            get
            {
                if (_eulaView == null)
                {
                    _eulaView = new EulaView(this);
                }
                return _eulaView;
            }
        }

        private EulaViewModel _eulaViewModel = null;
        public virtual EulaViewModel EulaViewModel
        {
            get
            {
                if (_eulaViewModel == null)
                {
                    _eulaViewModel = new EulaViewModel(this);
                }

                return _eulaViewModel;
            }
        }

        #endregion

        #region Install Dir

        private InstallDirView _installDirView = null;
        public virtual InstallDirView InstallDirView
        {
            get
            {
                if (_installDirView == null)
                {
                    _installDirView = new InstallDirView(this);
                }
                return _installDirView;
            }
        }

        private InstallDirViewModel _installDirViewModel = null;
        public virtual InstallDirViewModel InstallDirViewModel
        {
            get
            {
                if (_installDirViewModel == null)
                {
                    _installDirViewModel = new InstallDirViewModel(this);
                }

                return _installDirViewModel;
            }
        }

        #endregion

        #region SQL Account

        private DbAccountView _dbAccountView = null;
        public virtual DbAccountView DbAccountView
        {
            get
            {
                if (_dbAccountView == null)
                {
                    _dbAccountView = new DbAccountView(this);
                }
                return _dbAccountView;
            }
        }

        private DbAccountViewModel _dbAccountViewModel = null;
        public virtual DbAccountViewModel DbAccountViewModel
        {
            get
            {
                if (_dbAccountViewModel == null)
                {
                    _dbAccountViewModel = new DbAccountViewModel(this);
                }

                return _dbAccountViewModel;
            }
        }

        #endregion

        #region Progress

        private ProgressView _progressView = null;
        public virtual ProgressView ProgressView
        {
            get
            {
                if (_progressView == null)
                {
                    _progressView = new ProgressView(this);
                }
                return _progressView;
            }
        }

        private ProgressViewModel _progressViewModel = null;
        public virtual ProgressViewModel ProgressViewModel
        {
            get
            {
                if (_progressViewModel == null)
                {
                    _progressViewModel = new ProgressViewModel(this);
                }

                return _progressViewModel;
            }
        }

        #endregion

        #region Finish

        private FinishView _finishView = null;
        public virtual FinishView FinishView
        {
            get
            {
                if (_finishView == null)
                {
                    _finishView = new FinishView(this);
                }
                return _finishView;
            }
        }

        private FinishViewModel _finishViewModel = null;
        public virtual FinishViewModel FinishViewModel
        {
            get
            {
                if (_finishViewModel == null)
                {
                    _finishViewModel = new FinishViewModel(this);
                }

                return _finishViewModel;
            }
        }

        #endregion
    }
}