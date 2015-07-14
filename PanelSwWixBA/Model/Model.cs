//-------------------------------------------------------------------------------------------------
// <copyright file="Model.cs" company="Panel-SW.com">
//   Copyright (c) 2015, Panel-SW.com.
//   This software is released under Microsoft Reciprocal License (MS-RL).
//   The license and further copyright text can be found in the file
//   LICENSE.TXT at the root directory of the distribution.
// </copyright>
// 
// <summary>
// The model.
// </summary>
//-------------------------------------------------------------------------------------------------

namespace PanelSW.WixBA
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Net;
    using System.Reflection;
    using Microsoft.Tools.WindowsInstallerXml.Bootstrapper;
    using Microsoft.Win32;
    using System.Security;

    /// <summary>
    /// The model.
    /// </summary>
    public class Model
    {
        private Version version;
        private const string BurnBundleInstallDirectoryVariable = "ControlAppInstallFolder";
        private const string BurnBundleLayoutDirectoryVariable = "WixBundleLayoutDirectory";

        /// <summary>
        /// Creates a new model for the UX.
        /// </summary>
        /// <param name="bootstrapper">Bootstrapper hosting the UX.</param>
        public Model(BootstrapperApplication bootstrapper)
        {
            this.Bootstrapper = bootstrapper;
            this.Telemetry = new List<KeyValuePair<string, string>>();
        }

        /// <summary>
        /// Gets the bootstrapper.
        /// </summary>
        public BootstrapperApplication Bootstrapper { get; private set; }

        /// <summary>
        /// Gets the bootstrapper command-line.
        /// </summary>
        public Command Command { get { return this.Bootstrapper.Command; } }

        /// <summary>
        /// Gets the bootstrapper engine.
        /// </summary>
        public Engine Engine { get { return this.Bootstrapper.Engine; } }

        /// <summary>
        /// Gets the key/value pairs used in telemetry.
        /// </summary>
        public List<KeyValuePair<string, string>> Telemetry { get; private set; }

        /// <summary>
        /// Get or set the final result of the installation.
        /// </summary>
        public int Result { get; set; }

        /// <summary>
        /// Get the version of the install.
        /// </summary>
        public Version WixBundleVersion
        {
            get
            {
                if (null == this.version)
                {
                    this.version = Engine.VersionVariables["WixBundleVersion"];
                }

                return this.version;
            }
        }

        public String WixBundleName
        {
            get
            {
                return Engine.StringVariables["WixBundleName"];
            }
        }

        public String WixBundleManufacturer
        {
            get
            {
                return Engine.StringVariables["WixBundleManufacturer"];
            }
        }

        /// <summary>
        /// Get or set the path where the bundle is installed.
        /// </summary>
        public string InstallDirectory
        {
            get
            {
                string s = null;
                if (!this.Engine.StringVariables.Contains(BurnBundleInstallDirectoryVariable))
                {
                    return null;
                }

                s = this.Engine.StringVariables[BurnBundleInstallDirectoryVariable];
                if (s.Contains("["))
                {
                    s = Engine.FormatString(s);
                }

                return s;
            }

            set
            {
                this.Engine.StringVariables[BurnBundleInstallDirectoryVariable] = value;
            }
        }

        /// <summary>
        /// Get or set the path for the layout to be created.
        /// </summary>
        public string LayoutDirectory
        {
            get
            {
                if (!this.Engine.StringVariables.Contains(BurnBundleLayoutDirectoryVariable))
                {
                    return null;
                }

                return this.Engine.StringVariables[BurnBundleLayoutDirectoryVariable];
            }

            set
            {
                this.Engine.StringVariables[BurnBundleLayoutDirectoryVariable] = value;
            }
        }

        public LaunchAction PlannedAction { get; set; }

        /// <summary>
        /// Creates a correctly configured HTTP web request.
        /// </summary>
        /// <param name="uri">URI to connect to.</param>
        /// <returns>Correctly configured HTTP web request.</returns>
        public HttpWebRequest CreateWebRequest(string uri)
        {
            HttpWebRequest request = (HttpWebRequest)WebRequest.Create(uri);
            request.UserAgent = String.Concat("WixInstall", this.WixBundleVersion.ToString());

            return request;
        }

        #region SQL info

        public bool ShowSqlWindows
        {
            get
            {
                return
                    Engine.NumericVariables.Contains("HAS_SQL")
                    ? (Engine.NumericVariables["HAS_SQL"] != 0)
                    : false;
            }
            set
            {
                Engine.NumericVariables["HAS_SQL"] = value ? 1 : 0;
            }
        }

        public String SqlServer
        {
            get
            {
                return
                    Engine.StringVariables.Contains("SQL_SERVER")
                    ? Engine.StringVariables["SQL_SERVER"]
                    : "";
            }
            set
            {
                Engine.StringVariables["SQL_SERVER"] = value;
            }
        }

        public String SqlDbName
        {
            get
            {
                return
                    Engine.StringVariables.Contains("SQL_DATABASE")
                    ? Engine.StringVariables["SQL_DATABASE"]
                    : "";
            }
            set
            {
                Engine.StringVariables["SQL_DATABASE"] = value;
            }
        }

        public bool SqlAuth
        {
            get
            {
                return
                    Engine.NumericVariables.Contains("SQL_AUTH")
                    ? (Engine.NumericVariables["SQL_AUTH"] != 0)
                    : false;
            }
            set
            {
                Engine.NumericVariables["SQL_AUTH"] = value ? 1 : 0;
            }
        }

        public String SqlUserName
        {
            get
            {
                return
                    Engine.StringVariables.Contains("SQL_USERNAME")
                    ? Engine.StringVariables["SQL_USERNAME"]
                    : "";
            }
            set
            {
                Engine.StringVariables["SQL_USERNAME"] = value;
            }
        }

        public String SqlPassword
        {
            get
            {
                return
                    Engine.StringVariables.Contains("SQL_PASSWORD")
                    ? Engine.StringVariables["SQL_PASSWORD"]
                    : "";
            }
            set
            {
                Engine.StringVariables["SQL_PASSWORD"] = value;
            }
        }

        #endregion
    }
}
