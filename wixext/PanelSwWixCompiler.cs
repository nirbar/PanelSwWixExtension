namespace PanelSw.Wix.Extensions
{
    using Microsoft.Tools.WindowsInstallerXml;
    using System;
    using System.IO;
    using System.Reflection;
    using System.Xml;
    using System.Xml.Schema;

    /// <summary>
    /// The compiler for the Windows Installer XML Toolset PanelSwWixExtension Extension.
    /// </summary>
    public sealed class PanelSwWixCompiler : CompilerExtension
    {
        private XmlSchema schema;

        /// <summary>
        /// Instantiate a new PanelSwWixCompiler.
        /// </summary>
        public PanelSwWixCompiler()
        {
            schema = LoadXmlSchemaHelper(Assembly.GetExecutingAssembly(), "PanelSw.Wix.Extensions.Xsd.PanelSwWixExtension.xsd");
        }

        /// <summary>
        /// Gets the schema for this extension.
        /// </summary>
        /// <value>Schema for this extension.</value>
        public override XmlSchema Schema
        {
            get { return schema; }
        }

        public override void FinalizeCompile()
        {
            Core.EnsureTable(null, "PSW_CustomUninstallKey");
            Core.EnsureTable(null, "PSW_ReadIniValues");
            Core.EnsureTable(null, "PSW_RemoveRegistryValue");
            Core.EnsureTable(null, "PSW_XmlSearch");
            Core.EnsureTable(null, "PSW_Telemetry");
            Core.EnsureTable(null, "PSW_ShellExecute");
            Core.EnsureTable(null, "PSW_MsiSqlQuery");
            Core.EnsureTable(null, "PSW_RegularExpression");
            Core.EnsureTable(null, "PSW_FileRegex");
            Core.EnsureTable(null, "PSW_DeletePath"); 
            Core.EnsureTable(null, "PSW_TaskScheduler");
            Core.EnsureTable(null, "PSW_ExecOnComponent");
            Core.EnsureTable(null, "PSW_ExecOnComponent_ExitCode");
            Core.EnsureTable(null, "PSW_ZipFile"); 
            Core.EnsureTable(null, "PSW_Unzip"); 
            Core.EnsureTable(null, "PSW_ServiceConfig");
            Core.EnsureTable(null, "PSW_InstallUtil");
            Core.EnsureTable(null, "PSW_InstallUtil_Arg");
            Core.EnsureTable(null, "PSW_SqlSearch");
            base.FinalizeCompile();
        }

        /// <summary>
        /// Processes an element for the Compiler.
        /// </summary>
        /// <param name="sourceLineNumbers">Source line number for the parent element.</param>
        /// <param name="parentElement">Parent element of element to process.</param>
        /// <param name="element">Element to process.</param>
        /// <param name="contextValues">Extra information about the context in which this element is being parsed.</param>
        public override void ParseElement(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlElement element, params string[] contextValues)
        {
            switch (parentElement.LocalName)
            {
                case "Fragment":
                case "Module":
                case "Product":
                    switch (element.LocalName)
                    {
                        case "CustomUninstallKey":
                            ParseCustomUninstallKeyElement(element);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(element);
                            break;

                        case "RemoveRegistryValue":
                            ParseRemoveRegistryValue(element);
                            break;

                        case "Telemetry":
                            ParseTelemetry(element);
                            break;

                        case "ShellExecute":
                            ParseShellExecute(element);
                            break;

                        case "MsiSqlQuery":
                            ParseMsiSqlQuery(element);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(element);
                            break;

                        case "FileRegex":
                            ParseFileRegex(element);
                            break;

                        case "DeletePath":
                            ParseDeletePath(element);
                            break;

                        case "ZipFile":
                            ParseZipFile(element);
                            break;

                        case "Unzip":
                            ParseUnzip(element);
                            break;

                        case "SetPropertyFromPipe":
                            ParseSetPropertyFromPipe(element);
                            break;

                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Property":
                    switch (element.LocalName)
                    {
                        case "XmlSearch":
                            ParseXmlSearchElement(element);
                            break;

                        case "SqlSearch":
                            ParseSqlSearchElement(element);
                            break;

                        case "Evaluate":
                            ParseEvaluateElement(element);
                            break;

                        case "CertificateHashSearch":
                            ParseCertificateHashSearchElement(element);
                            break;

                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Component":
                    switch (element.LocalName)
                    {
                        case "TaskScheduler":
                            ParseTaskSchedulerElement(parentElement, element);
                            break;

                        case "ExecOn":
                        case "ExecOnComponent":
                            ParseExecOnComponentElement(parentElement, element);
                            break;

                        case "Dism":
                            ParseDismElement(parentElement, element);
                            break;

                        case "ServiceConfig":
                            ParseServiceConfigElement(parentElement, element);
                            break;

                        case "BackupAndRestore":
                            ParseBackupAndRestoreElement(parentElement, element);
                            break;

                        case "CreateSelfSignCertificate":
                            ParseCreateSelfSignCertificateElement(parentElement, element);
                            break;

                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "File":
                    switch (element.LocalName)
                    {
                        case "InstallUtil":
                            ParseInstallUtilElement(element, parentElement);
                            break;

                        case "TopShelf":
                            ParseTopShelfElement(element, parentElement);
                            break;

                        case "AlwaysOverwriteFile":
                            ParseAlwaysOverwriteFileElement(element, parentElement);
                            break;

                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;
                default:
                    Core.UnexpectedElement(parentElement, element);
                    break;
            }
        }

        private void ParseSetPropertyFromPipe(XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = "_" + Guid.NewGuid().ToString("N"); // Don't care about id.
            string pipe = null;
            int timeout = 0;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "PipeName":
                            pipe = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Timeout":
                            timeout = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, int.MaxValue);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "SetPropertyFromPipe");
            if (!Core.EncounteredError)
            {
                Core.EnsureTable(sourceLineNumbers, "PSW_SetPropertyFromPipe");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_SetPropertyFromPipe");
                row[0] = id;
                row[1] = pipe;
                row[2] = timeout;
            }
        }

        private void ParseCreateSelfSignCertificateElement(XmlElement parentElement, XmlElement element)
        {
            SourceLineNumberCollection parentsourceLineNumbers = Preprocessor.GetSourceLineNumbers(parentElement);
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string component = null;
            string password = null;
            string x500 = null;
            string subjectAltName = null;
            int expiry = 0;
            bool deleteOnCommit = true;

            component = Core.GetAttributeValue(parentsourceLineNumbers, parentElement.Attributes["Id"]);
            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(parentsourceLineNumbers, parentElement.Name, "Id"));
            }

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "password":
                            password = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "expiry":
                            expiry = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, int.MaxValue);
                            break;
                        case "x500":
                            x500 = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "deleteoncommit":
                            deleteOnCommit = (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;
                        case "subjectaltname":
                            subjectAltName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.Name, "Id"));
            }
            if (string.IsNullOrEmpty(x500))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "X500"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CreateSelfSignCertificate");
            if (!Core.EncounteredError)
            {
                Core.EnsureTable(sourceLineNumbers, "PSW_SelfSignCertificate");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_SelfSignCertificate");
                row[0] = id;
                row[1] = component;
                row[2] = x500;
                row[3] = subjectAltName;
                row[4] = expiry;
                row[5] = password;
                row[6] = deleteOnCommit ? 1 : 0;
            }
        }
        
        private enum BackupAndRestore_deferred_Schedule
        {
            BackupAndRestore_deferred_Before_InstallFiles,
            BackupAndRestore_deferred_After_DuplicateFiles,
            BackupAndRestore_deferred_After_RemoveExistingProducts
        }

        private void ParseBackupAndRestoreElement(XmlElement parentElement, XmlElement element)
        {
            SourceLineNumberCollection parentsourceLineNumbers = Preprocessor.GetSourceLineNumbers(parentElement);
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string filepath = null;
            string component = null;
            BackupAndRestore_deferred_Schedule restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
            DeletePathFlags flags = 0;

            component = Core.GetAttributeValue(parentsourceLineNumbers, parentElement.Attributes["Id"]);
            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(parentsourceLineNumbers, parentElement.Name, "Id"));
            }

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "path":
                            filepath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignoremissing":
                            if (Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= DeletePathFlags.IgnoreMissing;
                            }
                            break;
                        case "ignoreerrors":
                            if (Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= DeletePathFlags.IgnoreErrors;
                            }
                            break;
                        case "restorescheduling":
                            {
                                string val = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                switch (val)
                                {
                                    case "beforeInstallFiles":
                                        restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
                                        break;
                                    case "afterDuplicateFiles":
                                        restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_DuplicateFiles;
                                        break;
                                    case "afterRemoveExistingProducts":
                                        restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_After_RemoveExistingProducts;
                                        break;
                                    default:
                                    Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, element.LocalName, attrib.LocalName, val));
                                        break;
                                }
                            }
                            break;


                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.Name, "Id"));
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Path"));
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "BackupAndRestore");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "Property", restoreSchedule.ToString());

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Core.EnsureTable(sourceLineNumbers, "PSW_BackupAndRestore");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_BackupAndRestore");
                row[0] = id;
                row[1] = component;
                row[2] = filepath;
                row[3] = (int)flags;
            }
        }

        enum InstallUtil_Bitness
        {
            asComponent = 0,
            x86 = 1,
            x64 = 2
        }

        private void ParseInstallUtilElement(XmlNode node, XmlElement parentElement)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            InstallUtil_Bitness bitness = InstallUtil_Bitness.asComponent;

            string file = parentElement.GetAttribute("Id");
            if (string.IsNullOrEmpty(file))
            {
                file = parentElement.GetAttribute("Source");
                file = Path.GetFileName(file);
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "bitness":
                            string b = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                bitness = (InstallUtil_Bitness)Enum.Parse(typeof(InstallUtil_Bitness), b);
                            }
                            catch
                            {
                                Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.Name, "Id"));
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_InstallUtilSched");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_InstallUtil");
                row[0] = file;
                row[1] = (int)bitness;
            }

            // Iterate child 'Argument' elements
            foreach (XmlNode childNode in node.ChildNodes)
            {
                if (childNode.NodeType != XmlNodeType.Element)
                {
                    continue;
                }

                XmlElement child = childNode as XmlElement;
                if (!child.LocalName.Equals("Argument", StringComparison.OrdinalIgnoreCase))
                {
                    Core.UnsupportedExtensionElement(node, child);
                }

                string argId = null;
                string value = null;
                foreach (XmlAttribute attrib in child.Attributes)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            argId = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "value":
                            value = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }

                if (string.IsNullOrEmpty(argId))
                {
                    Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, child.Name, "Id"));
                    continue;
                }
                if (string.IsNullOrEmpty(value))
                {
                    Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, child.Name, "Value"));
                    continue;
                }

                // create a row in the PeriGen_PsfConfig table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_InstallUtil_Arg");
                row[0] = file;
                row[1] = argId;
                row[2] = value;
            }
        }

        private void ParseAlwaysOverwriteFileElement(XmlNode node, XmlElement parentElement)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);

            string file = parentElement.GetAttribute("Id");
            if (string.IsNullOrEmpty(file))
            {
                file = parentElement.GetAttribute("Source");
                file = Path.GetFileName(file);
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.Name, "Id"));
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.EnsureTable(sourceLineNumbers, "PSW_AlwaysOverwriteFile");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "AlwaysOverwriteFile");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_AlwaysOverwriteFile");
                row[0] = file;
            }
        }

        enum TopShelf_Account
        {
            custom = 0,
            localSystem = 1,
            localService = 2,
            networkService = 3,
            none = 4,
        }

        enum TopShelf_Start
        {
            disabled = 0,
            auto = 1,
            manual = 2,
            delayedAuto = 3,
            none = 4,
        }

        enum TopShelf_ErrorHandling
        {
            fail = 0,
            ignore = 1,
            prompt = 2
        }

        private void ParseTopShelfElement(XmlNode node, XmlElement parentElement)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            TopShelf_Account account = TopShelf_Account.none;
            TopShelf_Start start = TopShelf_Start.none;
            string serviceName = null;
            string displayName = null;
            string description = null;
            string instance = null;
            string userName = null;
            string password = null;
            TopShelf_ErrorHandling promptOnError = TopShelf_ErrorHandling.fail;

            string file = parentElement.GetAttribute("Id");
            if (string.IsNullOrEmpty(file))
            {
                file = parentElement.GetAttribute("Source");
                file = Path.GetFileName(file);
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Account":
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    account = (TopShelf_Account)Enum.Parse(typeof(TopShelf_Account), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                                }
                            }
                            break;

                        case "Start":
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    start = (TopShelf_Start)Enum.Parse(typeof(TopShelf_Start), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                                }
                            }
                            break;

                        case "ErrorHandling":
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    promptOnError = (TopShelf_ErrorHandling)Enum.Parse(typeof(TopShelf_ErrorHandling), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                                }
                            }
                            break;

                        case "ServiceName":
                            serviceName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "DisplayName":
                            displayName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Description":
                            description = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Instance":
                            instance = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "UserName":
                            userName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Password":
                            password = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.Name, "Id"));
            }
            if (string.IsNullOrEmpty(userName) != (account != TopShelf_Account.custom))
            {
                Core.OnMessage(WixErrors.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, "TopShelf", "Account", "custom", "UserName"));
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.EnsureTable(sourceLineNumbers, "PSW_TopShelf");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "TopShelf");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_TopShelf");
                row[0] = file;
                row[1] = serviceName;
                row[2] = displayName;
                row[3] = description;
                row[4] = instance;
                row[5] = (int)account;
                row[6] = userName;
                row[7] = password;
                row[8] = (int)start;
                row[9] = (int)promptOnError;
            }
        }

        [Flags]
        enum ExecOnComponentFlags
        {
            None = 0,

            // Action
            OnInstall = 1,
            OnRemove = 2 * OnInstall,
            OnReinstall = 2 * OnRemove,

            // Action rollback
            OnInstallRollback = 2 * OnReinstall,
            OnRemoveRollback = 2 * OnInstallRollback,
            OnReinstallRollback = 2 * OnRemoveRollback,

            AnyAction = OnInstall | OnRemove | OnReinstall | OnInstallRollback | OnReinstallRollback | OnRemoveRollback,

            // Schedule
            BeforeStopServices = 2 * OnReinstallRollback,
            AfterStopServices = 2 * BeforeStopServices,
            BeforeStartServices = 2 * AfterStopServices,
            AfterStartServices = 2 * BeforeStartServices,

            AnyTiming = BeforeStopServices | AfterStopServices | BeforeStartServices | AfterStartServices,

            // Return
            IgnoreExitCode = 2 * AfterStartServices,

            // Not waiting
            ASync = 2 * IgnoreExitCode,

            // Impersonate
            Impersonate = 2 * ASync,
        }

        private void ParseExecOnComponentElement(XmlElement parentElement, XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string component = null;
            string command = null;
            ExecOnComponentFlags flags = ExecOnComponentFlags.None;
            int order = 0;
            YesNoType aye;

            component = Core.GetAttributeValue(sourceLineNumbers, parentElement.Attributes["Id"]);

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                switch (attrib.LocalName)
                {
                    case "Id":
                        id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Command":
                        command = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Order":
                        order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 127);
                        break;

                    case "Impersonate":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.Impersonate;
                        }
                        break;

                    case "IgnoreExitCode":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.IgnoreExitCode;                                
                        }
                        break;

                    case "Wait":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.No)
                        {
                            flags |= ExecOnComponentFlags.ASync;
                        }
                        break;

                    case "OnInstall":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnInstall;
                        }
                        break;

                    case "OnInstallRollback":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnInstallRollback;
                        }
                        break;

                    case "OnReinstall":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnReinstall;
                        }
                        break;

                    case "OnReinstallRollback":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnReinstallRollback;
                        }
                        break;

                    case "OnUninstall":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnRemove;
                        }
                        break;

                    case "OnUninstallRollback":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnRemoveRollback;
                        }
                        break;

                    case "BeforeStopServices":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.BeforeStopServices;
                        }
                        break;

                    case "AfterStopServices":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.AfterStopServices;
                        }
                        break;

                    case "BeforeStartServices":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.BeforeStartServices;
                        }
                        break;

                    case "AfterStartServices":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.AfterStartServices;
                        }
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.Name, "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Id"));
            }
            if (string.IsNullOrEmpty(command))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Command"));
            }
            if ((flags & ExecOnComponentFlags.AnyAction) == ExecOnComponentFlags.None)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "OnXXX"));
            }
            if ((flags & ExecOnComponentFlags.AnyTiming) == ExecOnComponentFlags.None)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "BeforeXXX or AfterXXX"));
            }

            // ExitCode mapping
            foreach (XmlNode child in element.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch(child.LocalName)
                    {
                        case "ExitCode":
                            ushort from = 0, to = 0;
                            foreach (XmlAttribute a in child.Attributes)
                            {
                                if ((0 != a.NamespaceURI.Length) && (a.NamespaceURI != schema.TargetNamespace))
                                {
                                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, a);
                                }

                                switch (a.LocalName)
                                {
                                    case "Value":
                                        from = (ushort)Core.GetAttributeIntegerValue(sourceLineNumbers, a, 0, 0xffff);
                                        break;

                                    case "Behavior":
                                        switch (Core.GetAttributeValue(sourceLineNumbers, a))
                                        {
                                            case "success":
                                                to = 0;
                                                break;
                                            case "scheduleReboot":
                                                to = 3010;
                                                break;
                                            case "error":
                                                to = 0x4005;
                                                break;
                                            default:
                                                to = (ushort)Core.GetAttributeIntegerValue(sourceLineNumbers, a, 0, 0xffff);
                                                break;
                                        }
                                        break;
                                }
                            }

                            Row row = Core.CreateRow(sourceLineNumbers, "PSW_ExecOnComponent_ExitCode");
                            row[0] = id;
                            row[1] = (int)from;
                            row[2] = (int)to;
                            break;


                        default:
                            Core.UnexpectedElement(element, child);
                            break;
                    }
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ExecOnComponent");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ExecOnComponent");
                row[0] = id;
                row[1] = component;
                row[2] = command;
                row[3] = (int)flags;
                row[4] = order;
            }
        }

        private void ParseServiceConfigElement(XmlElement parentElement, XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string service = null;
            string account = null;
            string password = null;
            string component = null;

            component = Core.GetAttributeValue(sourceLineNumbers, parentElement.Attributes["Id"]);

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                switch (attrib.LocalName)
                {
                    case "Id":
                        id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ServiceName":
                        service = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Account":
                        account = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Password":
                        password = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            foreach (XmlNode child in element.ChildNodes)
            {
                Core.UnsupportedExtensionElement(element, child);
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedParentWithAttribute(sourceLineNumbers, parentElement.Name, "Id", ""));
            }
            if (string.IsNullOrEmpty(service))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "ServiceName"));
            }
            if (string.IsNullOrEmpty(account))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Account"));
            }
            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Id"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_ServiceConfig");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ServiceConfig");
                row[0] = id;
                row[1] = component;
                row[2] = service;
                row[3] = account;
                row[4] = password;
            }
        }

        private void ParseDismElement(XmlElement parentElement, XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string features = null;
            string exclude = null;
            string component = null;

            component = Core.GetAttributeValue(sourceLineNumbers, parentElement.Attributes["Id"]);

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                switch (attrib.LocalName)
                {
                    case "EnableFeature":
                        features = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ExcludeFeatures":
                        exclude = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Id":
                        id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            foreach (XmlNode child in element.ChildNodes)
            {
                Core.UnsupportedExtensionElement(element, child);
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedParentWithAttribute(sourceLineNumbers, parentElement.Name, "Id", ""));
            }
            if (string.IsNullOrEmpty(features))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "EnableFeature"));
            }
            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Id"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "DismSched");

            if (!Core.EncounteredError)
            {
                Core.EnsureTable(null, "PSW_Dism");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Dism");
                row[0] = id;
                row[1] = component;
                row[2] = features;
                row[3] = exclude;
            }
        }

        private void ParseTaskSchedulerElement(XmlElement parentElement, XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string taskXml = null;
            string taskName = null;
            string component = null;

            component = Core.GetAttributeValue(sourceLineNumbers, parentElement.Attributes["Id"]);

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }

                switch (attrib.LocalName)
                {
                    case "TaskName":
                        taskName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

           foreach (XmlNode child in element.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(element, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(element, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    taskXml = child.Value.Trim();
                    taskXml = taskXml.Replace(Environment.NewLine, "");
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedParentWithAttribute(sourceLineNumbers, parentElement.Name, "Id", ""));
            }
            if (string.IsNullOrEmpty(taskXml))
            {
                Core.OnMessage(WixErrors.ExpectedElement(sourceLineNumbers, element.Name, "Text or CDATA"));
            }
            if (string.IsNullOrEmpty(taskName))
            {
                Core.OnMessage(WixErrors.ExpectedElement(sourceLineNumbers, element.Name, "TaskName"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "TaskScheduler");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_TaskScheduler");
                row[0] = taskName;
                row[1] = component;
                row[2] = taskXml;
            }
        }

        [Flags]
        private enum CustomUninstallKeyAttributes
        {
            None = 0,
            Write = 1,
            Delete = 2
        }

        private void ParseCustomUninstallKeyElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string name = null;
            string data = null;
            string datatype = "REG_SZ";
            string id = null;
            string condition = null;
            CustomUninstallKeyAttributes attributes = CustomUninstallKeyAttributes.None;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (string.IsNullOrEmpty(name))
                            {
                                name = id;
                            }
                            break;
                        case "name":
                            name = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "data":
                            data = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "datatype":
                            datatype = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "operation":
                            if (Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("delete", StringComparison.OrdinalIgnoreCase))
                            {
                                attributes |= CustomUninstallKeyAttributes.Delete;
                            }
                            if (Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("write", StringComparison.OrdinalIgnoreCase))
                            {
                                attributes |= CustomUninstallKeyAttributes.Write;
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }

            if (string.IsNullOrEmpty(name))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Name"));
            }

            if (string.IsNullOrEmpty(data))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Data"));
            }

            if (string.IsNullOrEmpty(datatype))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "DataType"));
            }

            if (attributes == CustomUninstallKeyAttributes.None)
            {
                attributes = CustomUninstallKeyAttributes.Write;
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_Immediate");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_deferred");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_rollback");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_CustomUninstallKey");
                row[0] = id;
                row[1] = name;
                row[2] = data;
                row[3] = datatype;
                row[4] = (int)attributes;
                row[5] = condition;
            }
        }

        private enum ReadIniValuesAttributes
        {
            None = 0,
            IgnoreErrors = 1
        }
        
        private void ParseReadIniValuesElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string DestProperty = null;
            string FilePath = null;
            string Section = null;
            string Key = null;
            string IgnoreErrors = null;
            string condition = "";

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "destproperty":
                            DestProperty = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "filepath":
                            FilePath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "section":
                            Section = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "key":
                            Key = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignoreerrors":
                            IgnoreErrors = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }

            if (string.IsNullOrEmpty(DestProperty))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "DestProperty"));
            }

            if (string.IsNullOrEmpty(FilePath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "FilePath"));
            }

            if (string.IsNullOrEmpty(Key))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Key"));
            }

            if (string.IsNullOrEmpty(Section))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Section"));
            }

            // Attributes
            int Attributes = (int)ReadIniValuesAttributes.None;
            if (!string.IsNullOrEmpty(IgnoreErrors) && IgnoreErrors.Equals("yes", StringComparison.OrdinalIgnoreCase))
            {
                Attributes |= (int)ReadIniValuesAttributes.IgnoreErrors;
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ReadIniValues");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ReadIniValues");
                row[0] = id;
                row[1] = FilePath;
                row[2] = Section;
                row[3] = Key;
                row[4] = DestProperty;
                row[5] = Attributes;
                row[6] = condition;
            }
        }

        private enum RegistryArea
        {
            x86,
            x64,
            Default
        }

        private void ParseRemoveRegistryValue(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string root = null;
            string key = null;
            string name = null;
            RegistryArea area = RegistryArea.Default;
            string condition = "";

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "root":
                            root = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "key":
                            key = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "name":
                            name = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "area":
                            try
                            {
                                area = (RegistryArea)Enum.Parse(typeof(RegistryArea), Core.GetAttributeValue(sourceLineNumbers, attrib));
                            }
                            catch
                            {
                                Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, node.Name, "Area", Core.GetAttributeValue(sourceLineNumbers, attrib)));
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(key))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Key"));
            }
            if (string.IsNullOrEmpty(root))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Root"));
            }
            if (string.IsNullOrEmpty(name))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Name"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "RemoveRegistryValue_Immediate");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_RemoveRegistryValue");
                row[0] = id;
                row[1] = root;
                row[2] = key;
                row[3] = name;
                row[4] = area.ToString();
                row[5] = 0;
                row[6] = condition;
            }
        }

        private void ParseCertificateHashSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string property = null;
            string certName = null;
            string issuer = null;
            string serial = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "CertName":
                            certName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Issuer":
                            issuer = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "SerialNumber":
                            serial = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }

            // Either CertName OR (Issuer AND Serial)
            if (string.IsNullOrEmpty(issuer) != string.IsNullOrEmpty(serial))
            {
                Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, node.Name, "Issuer", "SerialNumber"));
            }
            if (string.IsNullOrEmpty(certName) == string.IsNullOrEmpty(issuer))
            {
                Core.OnMessage(WixErrors.ExpectedAttributesWithoutOtherAttribute(sourceLineNumbers, node.Name, "Issuer", "SerialNumber", "CertName"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            if (!Core.EncounteredError)
            {
                Core.EnsureTable(sourceLineNumbers, "PSW_CertificateHashSearch");
                Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CertificateHashSearch");

                Row row = Core.CreateRow(sourceLineNumbers, "PSW_CertificateHashSearch");
                row[0] = property;
                row[1] = certName;
                row[2] = issuer;
                row[3] = serial;
            }
        }

        private void ParseEvaluateElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "_" + Guid.NewGuid().ToString("N");
            string property = null;
            string expression = null;
            int order = 0;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Expression":
                            expression = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 255);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Expression"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.EnsureTable(sourceLineNumbers, "PSW_EvaluateExpression");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "EvaluateExpression");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_EvaluateExpression");
                row[0] = id;
                row[1] = property;
                row[2] = expression;
                row[3] = order;
            }
        }

        private enum XmlSearchMatch
        {
            first,
            all,
            enforceSingle
        }

        private void ParseXmlSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string filePath = null;
            string xpath = null;
            string property = null;
            string lang = null;
            string namespaces = null;
            XmlSearchMatch match = XmlSearchMatch.first;
            string condition = "";

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "filepath":
                            filePath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "xpath":
                            xpath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "language":
                            lang = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "namespaces":
                            namespaces = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "match":
                            try
                            {
                                match = (XmlSearchMatch)Enum.Parse(typeof(XmlSearchMatch), Core.GetAttributeValue(sourceLineNumbers, attrib));
                            }
                            catch
                            {
                                Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, node.Name, "Match", Core.GetAttributeValue(sourceLineNumbers, attrib)));
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(filePath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "FilePath"));
            }
            if (string.IsNullOrEmpty(xpath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "XPath"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "XmlSearch");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_XmlSearch");
                row[0] = id;
                row[1] = property;
                row[2] = filePath;
                row[3] = xpath;
                row[4] = lang;
                row[5] = namespaces;
                row[6] = match.ToString();
                row[7] = 0;
                row[8] = condition;
            }
        }

        private void ParseSqlSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string server = null;
            string instance = null;
            string database = null;
            string username = null;
            string password = null;
            string query = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            id = node.ParentNode.Attributes["Id"].Value;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Server":
                            server = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Instance":
                            instance = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Database":
                            database = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Username":
                            username = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Password":
                            password = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(server))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "FilePath"));
            }
            if (string.IsNullOrEmpty(query))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "XPath"));
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "SqlSearch");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_SqlSearch");
                row[0] = id;
                row[1] = server;
                row[2] = instance;
                row[3] = database;
                row[4] = username;
                row[5] = password;
                row[6] = query;
            }
        }

        [Flags]
        private enum ExecutePhase
        {
            None = 0,
            OnExecute = 1,
            OnCommit = 2,
            OnRollback = 4,
            Secure = 8
        }

        private void ParseTelemetry(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string url = null;
            string page = null;
            string method = null;
            string data = null;
            ExecutePhase flags = ExecutePhase.None;
            string condition = "";

            foreach (XmlAttribute attrib in node.Attributes)
            {
                string tmp;
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "url":
                            url = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "page":
                            page = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "method":
                            method = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "data":
                            data = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "onsuccess":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if( tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "onstart":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if( tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "onfailure":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if( tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;
                        case "secure":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.Secure;
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(url))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Url"));
            }
            if (string.IsNullOrEmpty(method))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Method"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "Telemetry");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Telemetry");
                row[0] = id;
                row[1] = url;
                row[2] = page ?? "";
                row[3] = method;
                row[4] = data ?? "";
                row[5] = (int)flags;
                row[6] = condition;
            }
        }

        private void ParseShellExecute(XmlElement node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string target = null;
            string args = "";
            string workDir = "";
            string verb = "";
            int wait = 0;
            int show = 0;
            ExecutePhase flags = ExecutePhase.None;
            string condition = "";

            foreach (XmlAttribute attrib in node.Attributes)
            {
                string tmp;
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "target":
                            target = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "args":
                            args = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "workingdir":
                            workDir = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "verb":
                            verb = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "wait":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                wait = 1;
                            }
                            break;
                        case "show":
                            show = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 15);
                            break;

                        case "oncommit":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "onexecute":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "onrollback":
                            tmp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(target))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Target"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // Default to execute deferred.
            if (flags == ExecutePhase.None)
            {
                flags = ExecutePhase.OnExecute;
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ShellExecute_Immediate");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                // `Id`, `Target`, `Args`, `Verb`, `WorkingDir`, `Show`, `Wait`, `Condition`
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ShellExecute");
                row[0] = id;
                row[1] = target;
                row[2] = args;
                row[3] = verb;
                row[4] = workDir;
                row[5] = show;
                row[6] = wait;
                row[7] = (int)flags;
                row[8] = condition;
            }
        }

        private void ParseMsiSqlQuery(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string query = null;
            string condition = null;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "query":
                            query = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }

            if (string.IsNullOrEmpty(query))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Query"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "MsiSqlQuery");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_MsiSqlQuery");
                row[0] = id;
                row[1] = query;
                row[2] = condition;
            }
        }

        [Flags]
        enum RegexSearchFlags
        {
            Search = 0
            , Replace = 1
        };

        [Flags]
        enum RegexResultFlags
        {
            MustMatch = 1
        };

        [Flags]
        enum RegexMatchFlags
        {
            IgnoreCare = 1
            , Extended = 2
        };

        private void ParseRegularExpression(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string input = null;
            string regex = null;
            string replacement = null;
            string prop = null;
            int flags = 0;
            string condition = null;
            byte order = 0xFF;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "input":
                            input = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "expression":
                            regex = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "replacement":
                            replacement = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            flags |= (int)RegexSearchFlags.Replace;
                            break;
                        case "dstproperty":
                            prop = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignorecase":
                            flags |= (int)RegexMatchFlags.IgnoreCare << 2;
                            break;
                        case "extended":
                            flags |= (int)RegexMatchFlags.Extended << 2;
                            break;
                        case "order":
                            order = (byte)Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 0xFF);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(input))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Input"));
            }
            if (string.IsNullOrEmpty(regex))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Expression"));
            }
            if (string.IsNullOrEmpty(prop))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "DstProperty"));
            }
            if (string.IsNullOrEmpty(input))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Input"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "RegularExpression");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_RegularExpression");
                row[0] = id;
                row[1] = input;
                row[2] = regex;
                row[3] = replacement;
                row[4] = prop;
                row[5] = flags;
                row[6] = condition;
                row[7] = (int)order;
            }
        }

        private enum FileEncoding
        {
            AutoDetect,
            MultiByte,
            Unicode,
            ReverseUnicode
        };

        private void ParseFileRegex(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string regex = null;
            string replacement = null;
            FileEncoding encoding = FileEncoding.AutoDetect;
            bool ignoreCase = false;
            string condition = null;
            byte order = 0xFF;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "filepath":
                            filepath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "regex":
                            regex = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "replacement":
                            replacement = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignorecase":
                            ignoreCase = true;
                            break;
                        case "encoding":
                            string enc = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            encoding = (FileEncoding)Enum.Parse(typeof(FileEncoding), enc);
                            break;
                        case "order":
                            order = (byte)Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 0xFF);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "FilePath"));
            }
            if (string.IsNullOrEmpty(regex))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Regex"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "FileRegex_Immediate");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_FileRegex");
                row[0] = id;
                row[1] = filepath;
                row[2] = regex;
                row[3] = replacement ?? "";
                row[4] = ignoreCase ? 1 : 0;
                row[5] = (int)encoding;
                row[6] = condition;
                row[7] = (int)order;
            }
        }

        [Flags]
        private enum DeletePathFlags
        {
            IgnoreMissing = 1
            , IgnoreErrors = 2 * IgnoreMissing
        }

        private void ParseDeletePath(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string condition = null;
            DeletePathFlags flags = 0;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "path":
                            filepath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignoremissing":
                            if (Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= DeletePathFlags.IgnoreMissing;
                            }
                            break;
                        case "ignoreerrors":
                            if (Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= DeletePathFlags.IgnoreErrors;
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Path"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "DeletePath");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_DeletePath");
                row[0] = id;
                row[1] = filepath;
                row[2] = (int)flags;
                row[3] = condition;
            }
        }

        private void ParseZipFile(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string dstZipFile = null;
            string srcDir = null;
            string filePattern = "*.*";
            bool recursive = true;
            string condition = null;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "TargetZipFile":
                            dstZipFile = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "SourceFolder":
                            srcDir = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePattern":
                            filePattern = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Recursive":
                            recursive = (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(dstZipFile))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "TargetZipFile"));
            }
            if (string.IsNullOrEmpty(srcDir))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "SourceFolder"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ZipFileSched");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ZipFile");
                row[0] = id;
                row[1] = dstZipFile;
                row[2] = srcDir;
                row[3] = filePattern;
                row[4] = recursive ? 1 : 0;
                row[5] = condition;
            }
        }

        [Flags]
        private enum UnzipFlags
        {
            None = 0,
            Overwrite = 1
        }
        private void ParseUnzip(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string zipFile = null;
            string dstDir = null;
            string condition = null;
            YesNoType aye = YesNoType.No;
            UnzipFlags flags = UnzipFlags.None;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ZipFile":
                            zipFile = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "TargetFolder":
                            dstDir = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Overwrite":
                            aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                flags |= UnzipFlags.Overwrite;                                    
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(zipFile))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "ZipFile"));
            }
            if (string.IsNullOrEmpty(dstDir))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "TargetFolder"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == schema.TargetNamespace)
                    {
                        Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "UnzipSched");

            if (!Core.EncounteredError)
            {
                // create a row in the Win32_CopyFiles table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Unzip");
                row[0] = id;
                row[1] = zipFile;
                row[2] = dstDir;
                row[3] = (int)flags;
                row[4] = condition;
            }
        }
    }
}
