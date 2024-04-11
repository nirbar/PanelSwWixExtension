using Microsoft.Tools.WindowsInstallerXml;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Xml;
using System.Xml.Schema;

namespace PanelSw.Wix.Extensions
{
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

                        case "DuplicateFolder":
                            ParseDuplicateFolderElement(element);
                            break;

                        case "Payload":
                            ParsePayload(element, null, null);
                            break;

                        case "FileRegex":
                            ParseFileRegex(element, null, null);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(element, parentElement);
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
                            ParseMsiSqlQuery(element, parentElement);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(element);
                            break;

                        case "RestartLocalResources":
                            ParseRestartLocalResources(element);
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

                        case "ExecuteCommand":
                            ParseExecuteCommand(element);
                            break;

                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "DirectoryRef":
                case "Directory":
                    {
                        string directoryId = contextValues[0];
                        string diskId = contextValues[1];

                        switch (element.LocalName)
                        {
                            case "DiskSpace":
                                ParseDiskSpaceElement(element, directoryId);
                                break;

                            default:
                                Core.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "Property":
                    switch (element.LocalName)
                    {
                        case "AccountSidSearch":
                            ParseAccountSidSearchElement(element);
                            break;

                        case "PathSearch":
                            ParsePathSearchElement(element);
                            break;

                        case "VersionCompare":
                            ParseVersionCompareElement(element);
                            break;

                        case "XmlSearch":
                            ParseXmlSearchElement(element);
                            break;

                        case "WmiSearch":
                            ParseWmiSearchElement(element);
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

                        case "JsonJpathSearch":
                            ParseJsonJpathSearchElement(element);
                            break;

                        case "MsiSqlQuery":
                            ParseMsiSqlQuery(element, parentElement);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(element, parentElement);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(element);
                            break;

                        case "ToLowerCase":
                            ParseToLowerCase(element);
                            break;

                        case "Md5Hash":
                            ParseMd5Hash(element);
                            break;

                        case "IsWindowsVersionOrGreater":
                            ParseIsWindowsVersionOrGreater(element);
                            break;

                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Component":
                    {
                        string componentId = contextValues[0];
                        string directoryId = contextValues[1];
                        bool isWin64 = bool.Parse(contextValues[2]);

                        switch (element.LocalName)
                        {
                            case "JsonJPath":
                                ParseJsonJPathElement(element, componentId, null);
                                break;

                            case "TaskScheduler":
                                ParseTaskSchedulerElement(element, componentId);
                                break;

                            case "ExecOn":
                            case "ExecOnComponent":
                                ParseExecOnComponentElement(element, componentId);
                                break;

                            case "Dism":
                                ParseDismElement(element, componentId);
                                break;

                            case "ServiceConfig":
                                ParseServiceConfigElement(element, componentId);
                                break;

                            case "BackupAndRestore":
                                ParseBackupAndRestoreElement(element, componentId);
                                break;

                            case "CreateSelfSignCertificate":
                                ParseCreateSelfSignCertificateElement(element, componentId);
                                break;

                            case "SqlScript":
                                ParseSqlScriptElement(element, componentId);
                                break;

                            case "WebsiteConfig":
                                ParseWebsiteConfigElement(element, componentId);
                                break;

                            case "XslTransform":
                                ParseXslTransform(element, componentId, null);
                                break;

                            default:
                                Core.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "File":
                    {
                        string fileId = contextValues[0];
                        string componentId = contextValues[1];
                        bool isWin64 = bool.Parse(contextValues[2]);
                        string directoryId = contextValues[3];

                        switch (element.LocalName)
                        {
                            case "JsonJPath":
                                ParseJsonJPathElement(element, componentId, fileId);
                                break;

                            case "InstallUtil":
                                ParseInstallUtilElement(element, fileId);
                                break;

                            case "TopShelf":
                                ParseTopShelfElement(element, fileId);
                                break;

                            case "AlwaysOverwriteFile":
                            case "ForceVersion":
                                ParseForceVersionElement(element, fileId);
                                break;

                            case "FileRegex":
                                ParseFileRegex(element, componentId, fileId);
                                break;

                            case "XslTransform":
                                ParseXslTransform(element, componentId, fileId);
                                break;

                            case "SplitFile":
                                ParseSplitFileElement(parentElement, element, componentId, fileId);
                                break;

                            default:
                                Core.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "PatchFamily":
                    switch (element.LocalName)
                    {
                        case "CustomPatchRef":
                            ParseCustomPatchRefElement(element);
                            break;
                        default:
                            Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "ExePackage":
                    string pkgId = contextValues[0];
                    switch (element.LocalName)
                    {
                        case "UninstallCommand":
                            ParseUninstallCommandElement(parentElement, element, pkgId);
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

        private void ParseDuplicateFolderElement(XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string sourceDir = null;
            string destDir = null;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "SourceDir":
                        sourceDir = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;
                    case "DestinationDir":
                        destDir = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;
                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(sourceDir))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "SourceDir"));
            }
            if (string.IsNullOrEmpty(destDir))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "DestinationDir"));
            }

            if (Core.EncounteredError)
            {
                return;
            }

            Row myComponentGroupRow = Core.CreateRow(sourceLineNumbers, "WixComponentGroup");
            myComponentGroupRow[0] = $"cg{Guid.NewGuid().ToString("N")}";

            Core.CreateWixGroupRow(sourceLineNumbers, ComplexReferenceParentType.Unknown, null, ComplexReferenceChildType.ComponentGroup, myComponentGroupRow[0].ToString());
            Core.CreateComplexReference(sourceLineNumbers, ComplexReferenceParentType.ComponentGroup, myComponentGroupRow[0].ToString(), "WiX", ComplexReferenceChildType.ComponentGroup, "PSW_DuplicateFile", false);

            Core.EnsureTable(sourceLineNumbers, "CreateFolder");
            Core.EnsureTable(sourceLineNumbers, "DuplicateFile");

            Row dupRow = Core.CreateRow(sourceLineNumbers, "PSW_DuplicateFolder");
            dupRow[0] = sourceDir;
            dupRow[1] = destDir;
        }

        private void ParsePayload(XmlElement element, object p1, object p2)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string source = null;
            string name = null;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Source":
                        source = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Name":
                        name = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(source))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Source"));
                return;
            }
            if (string.IsNullOrEmpty(name))
            {
                name = Path.GetFileName(source);
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ExtractPayload");

            string binaryKey = $"pld{Guid.NewGuid().ToString("N")}";
            Row binaryRow = Core.CreateRow(sourceLineNumbers, "Binary");
            binaryRow[0] = binaryKey;
            binaryRow[1] = source;

            Row pldRow = Core.CreateRow(sourceLineNumbers, "PSW_Payload");
            pldRow[0] = binaryKey;
            pldRow[1] = name;
        }

        private void ParseUninstallCommandElement(XmlElement parentElement, XmlElement element, string pkgId)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = $"{pkgId}_uninstall";
            string detectCondition = null;
            string commandLine = null;

            string nmspc = parentElement.OwnerDocument.GetPrefixOfNamespace(parentElement.NamespaceURI);
            if (!string.IsNullOrEmpty(nmspc))
            {
                nmspc = parentElement.NamespaceURI;
            }

            if (parentElement.HasAttribute("DetectCondition", nmspc))
            {
                detectCondition = parentElement.GetAttribute("DetectCondition", nmspc);
            }

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Id":
                        id = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;

                    case "DetectCondition":
                        detectCondition = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "CommandLine":
                        commandLine = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(commandLine))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "CommandLine"));
            }
            if (string.IsNullOrEmpty(detectCondition))
            {
                Core.OnMessage(WixErrors.ExpectedAttributeInElementOrParent(sourceLineNumbers, element.LocalName, "DetectCondition", parentElement.LocalName));
            }

            if (!Core.EncounteredError)
            {
                XmlProcessingInstruction sourceLineElement = element.OwnerDocument.CreateProcessingInstruction(Preprocessor.LineNumberElementName, sourceLineNumbers.EncodedSourceLineNumbers);
                parentElement.ParentNode.InsertAfter(sourceLineElement, parentElement);

                XmlElement uninstallElement = element.OwnerDocument.CreateElement("ExePackage", parentElement.NamespaceURI);
                uninstallElement.SetAttribute("Id", nmspc, id);
                uninstallElement.SetAttribute("SourceFile", nmspc, "!(bindpath.PanelSwWixExtension)\\DeferredExePackage.exe");
                uninstallElement.SetAttribute("After", nmspc, pkgId);
                uninstallElement.SetAttribute("DetectCondition", nmspc, detectCondition);
                uninstallElement.SetAttribute("InstallCommand", nmspc, "--ignore-me");
                uninstallElement.SetAttribute("RepairCommand", nmspc, "--ignore-me");
                uninstallElement.SetAttribute("UninstallCommand", nmspc, $"--skip-until-here {commandLine}");
                uninstallElement.SetAttribute("Cache", nmspc, "always");
                uninstallElement.SetAttribute("Compressed", nmspc, "yes");
                string[] copyAttributes = new string[] { "InstallCondition", "DisplayName", "Vital", "Description", "PerMachine" };
                foreach (string att in copyAttributes)
                {
                    if (parentElement.HasAttribute(att, nmspc))
                    {
                        uninstallElement.SetAttribute(att, nmspc, parentElement.GetAttribute(att, nmspc));
                    }
                }

                parentElement.ParentNode.InsertAfter(uninstallElement, sourceLineElement);
            }
        }

        private void ParseSplitFileElement(XmlElement fileElement, XmlElement element, string componentId, string fileId)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            int splitSize = Int32.MaxValue; //2GB

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Size":
                        splitSize = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 1, Int32.MaxValue);
                        break;
                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            XmlAttribute sourceAttrib = fileElement.GetAttributeNode("Source");
            if (sourceAttrib == null)
            {
                sourceAttrib = fileElement.GetAttributeNode("src");
            }
            string sourcePath = Core.GetAttributeValue(sourceLineNumbers, sourceAttrib);
            if (!File.Exists(sourcePath))
            {
                Core.OnMessage(WixErrors.FileNotFound(sourceLineNumbers, sourcePath));
                return;
            }
            FileInfo fileInfo = new FileInfo(sourcePath);
            if (fileInfo.Length <= splitSize)
            {
                return;
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ConcatFiles");

            string tmpPath = Path.GetTempPath();
            int splitCnt = (int)Math.Ceiling(1m * fileInfo.Length / splitSize);
            for (int i = 1; i < splitCnt; ++i)
            {
                XmlElement splitFileElement = fileElement.CloneNode(false) as XmlElement;
                string splId = "spl" + Guid.NewGuid().ToString("N");
                string splFile = Path.Combine(tmpPath, splId);
                File.Create(splFile).Dispose();

                string nmspc = fileElement.OwnerDocument.GetPrefixOfNamespace(fileElement.NamespaceURI);
                if (!string.IsNullOrEmpty(nmspc))
                {
                    nmspc = fileElement.NamespaceURI;
                }

                splitFileElement.SetAttribute("KeyPath", nmspc, "no");
                splitFileElement.SetAttribute("CompanionFile", nmspc, fileId);
                splitFileElement.SetAttribute("Name", nmspc, splId);
                splitFileElement.SetAttribute("Id", nmspc, splId);
                splitFileElement.SetAttribute("Source", nmspc, splFile);
                splitFileElement.RemoveAttribute("src", nmspc);

                fileElement.ParentNode.InsertAfter(splitFileElement, fileElement);

                if (!Core.EncounteredError)
                {
                    Row row = Core.CreateRow(sourceLineNumbers, "PSW_ConcatFiles");
                    row[0] = componentId;
                    row[1] = fileId;
                    row[2] = splId;
                    row[3] = i;
                    row[4] = splitSize;
                }
            }
        }

        private void ParseCustomPatchRefElement(XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string table = null;
            string key = null;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Table":
                        table = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;
                    case "Key":
                        key = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;
                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(table))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Table"));
            }
            if (string.IsNullOrEmpty(key))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Key"));
            }

            // When referencing a table row, reference the relevant custom actions
            List<string> customActions = new List<string>();
            switch (table)
            {
                case "PSW_XmlSearch":
                    customActions.Add("XmlSearch");
                    break;
                case "PSW_FileRegex":
                    customActions.Add("FileRegex_Immediate");
                    customActions.Add("FileRegex_rollback");
                    customActions.Add("FileRegex_deferred");
                    customActions.Add("FileRegex_commit");
                    break;
                case "PSW_CustomUninstallKey":
                    customActions.Add("CustomUninstallKey_Immediate");
                    customActions.Add("CustomUninstallKey_deferred");
                    customActions.Add("CustomUninstallKey_rollback");
                    break;
                case "PSW_ReadIniValues":
                    customActions.Add("ReadIniValues");
                    break;
                case "PSW_RemoveRegistryValue":
                    customActions.Add("RemoveRegistryValue_Immediate");
                    customActions.Add("RemoveRegistryValue_deferred");
                    customActions.Add("RemoveRegistryValue_rollback");
                    break;
                case "PSW_RegularExpression":
                    customActions.Add("RegularExpression");
                    break;
                case "PSW_Telemetry":
                    customActions.Add("Telemetry");
                    customActions.Add("Telemetry_deferred");
                    customActions.Add("Telemetry_rollback");
                    customActions.Add("Telemetry_commit");
                    break;
                case "PSW_ShellExecute":
                    customActions.Add("ShellExecute_Immediate");
                    customActions.Add("ShellExecute_deferred");
                    customActions.Add("ShellExecute_rollback");
                    customActions.Add("ShellExecute_commit");
                    break;
                case "PSW_MsiSqlQuery":
                    customActions.Add("MsiSqlQuery");
                    break;
                case "PSW_DeletePath":
                    customActions.Add("DeletePath");
                    customActions.Add("DeletePath_rollback");
                    customActions.Add("DeletePath_deferred");
                    customActions.Add("DeletePath_commit");
                    break;
                case "PSW_TaskScheduler":
                    customActions.Add("TaskScheduler");
                    customActions.Add("TaskScheduler_rollback");
                    customActions.Add("TaskScheduler_deferred");
                    customActions.Add("TaskScheduler_commit");
                    break;
                case "PSW_ExecOnComponent":
                case "PSW_ExecOnComponent_ExitCode":
                case "PSW_ExecOn_ConsoleOutput":
                case "PSW_ExecOnComponent_Environment":
                    customActions.Add("ExecOnComponent");
                    customActions.Add("ExecOnComponentRollback");
                    customActions.Add("ExecOnComponentCommit");
                    customActions.Add("ExecOnComponent_BeforeStop_rollback");
                    customActions.Add("ExecOnComponent_BeforeStop_deferred");
                    customActions.Add("ExecOnComponent_AfterStop_rollback");
                    customActions.Add("ExecOnComponent_AfterStop_deferred");
                    customActions.Add("ExecOnComponent_BeforeStart_rollback");
                    customActions.Add("ExecOnComponent_BeforeStart_deferred");
                    customActions.Add("ExecOnComponent_AfterStart_rollback");
                    customActions.Add("ExecOnComponent_AfterStart_deferred");
                    break;
                case "PSW_Dism":
                    customActions.Add("DismSched");
                    customActions.Add("DismX86");
                    customActions.Add("DismX64");
                    break;
                case "PSW_ZipFile":
                    customActions.Add("ZipFileSched");
                    customActions.Add("ZipFileExec");
                    break;
                case "PSW_Unzip":
                    customActions.Add("UnzipSched");
                    customActions.Add("UnzipExec");
                    break;
                case "PSW_ServiceConfig":
                case "PSW_ServiceConfig_Dependency":
                    customActions.Add("PSW_ServiceConfig");
                    customActions.Add("PSW_ServiceConfigRlbk");
                    customActions.Add("PSW_ServiceConfigExec");
                    break;
                case "PSW_InstallUtil":
                case "PSW_InstallUtil_Arg":
                    customActions.Add("PSW_InstallUtilSched");
                    customActions.Add("PSW_InstallUtil_InstallExec_x86");
                    customActions.Add("PSW_InstallUtil_InstallRollback_x86");
                    customActions.Add("PSW_InstallUtil_UninstallExec_x86");
                    customActions.Add("PSW_InstallUtil_UninstallRollback_x86");
                    customActions.Add("PSW_InstallUtil_InstallExec_x64");
                    customActions.Add("PSW_InstallUtil_InstallRollback_x64");
                    customActions.Add("PSW_InstallUtil_UninstallExec_x64");
                    customActions.Add("PSW_InstallUtil_UninstallRollback_x64");
                    break;
                case "PSW_SqlSearch":
                    customActions.Add("SqlSearch");
                    break;
                case "PSW_BackupAndRestore":
                    customActions.Add("BackupAndRestore");
                    customActions.Add("BackupAndRestore_rollback");
                    customActions.Add("BackupAndRestore_deferred");
                    customActions.Add("BackupAndRestore_commit");
                    break;
                case "PSW_TopShelf":
                    customActions.Add("TopShelf");
                    customActions.Add("TopShelfService_InstallRollback");
                    customActions.Add("TopShelfService_Install");
                    customActions.Add("TopShelfService_UninstallRollback");
                    customActions.Add("TopShelfService_Uninstall");
                    break;
                case "PSW_SelfSignCertificate":
                    customActions.Add("CreateSelfSignCertificate");
                    customActions.Add("CreateSelfSignCertificate_commit");
                    break;
                case "PSW_ForceVersion":
                    customActions.Add("ForceVersion");
                    break;
                case "PSW_SetPropertyFromPipe":
                    customActions.Add("SetPropertyFromPipe");
                    break;
                case "PSW_EvaluateExpression":
                    customActions.Add("EvaluateExpression");
                    break;
                case "PSW_CertificateHashSearch":
                    customActions.Add("CertificateHashSearch");
                    break;
                case "PSW_DiskSpace":
                    customActions.Add("DiskSpace");
                    break;
                case "PSW_JsonJPath":
                    customActions.Add("JsonJpathSched");
                    customActions.Add("JsonJpathExec");
                    break;
                case "PSW_JsonJpathSearch":
                    customActions.Add("JsonJpathSearch");
                    break;
                case "PSW_AccountSidSearch":
                    customActions.Add("AccountSidSearch");
                    break;
                case "PSW_XslTransform":
                case "PSW_XslTransform_Replacements":
                    customActions.Add("PSW_XslTransform");
                    customActions.Add("PSW_XslTransformExec");
                    break;
                case "PSW_SqlScript":
                case "PSW_SqlScript_Replacements":
                    customActions.Add("PSW_SqlScript");
                    customActions.Add("PSW_SqlScriptRollback");
                    customActions.Add("PSW_SqlScriptExec");
                    break;
                case "PSW_WebsiteConfig":
                    customActions.Add("PSW_WebsiteConfigSched");
                    customActions.Add("PSW_WebsiteConfigExec");
                    break;
                case "PSW_VersionCompare":
                    customActions.Add("PSW_VersionCompare");
                    break;
                case "PSW_PathSearch":
                    customActions.Add("PSW_PathSearch");
                    break;
                case "PSW_ToLowerCase":
                    customActions.Add("PSW_ToLowerCase");
                    break;
                case "PSW_WmiSearch":
                    customActions.Add("WmiSearch");
                    break;
                case "PSW_RestartLocalResources":
                    customActions.Add("RestartLocalResources");
                    customActions.Add("RestartLocalResourcesExec"); break;
                case "PSW_Md5Hash":
                    customActions.Add("Md5Hash");
                    break;
                case "PSW_IsWindowsVersionOrGreater":
                    customActions.Add("IsWindowsVersionOrGreater");
                    break;
                case "PSW_ConcatFiles":
                    customActions.Add("ConcatFiles");
                    customActions.Add("ConcatFilesExec");
                    break;
                case "PSW_Payload":
                    customActions.Add("ExtractPayload");
                    customActions.Add("ExtractPayloadRollback");
                    customActions.Add("ExtractPayloadCommit");
                    break;
            }

            if (!Core.EncounteredError)
            {
                Row patchReferenceRow;
                foreach (string ca in customActions)
                {
                    patchReferenceRow = Core.CreateRow(sourceLineNumbers, "WixPatchRef");
                    patchReferenceRow[0] = "CustomAction";
                    patchReferenceRow[1] = ca;
                }

                patchReferenceRow = Core.CreateRow(sourceLineNumbers, "WixPatchRef");
                patchReferenceRow[0] = "Binary";
                patchReferenceRow[1] = "PanelSwCustomActions.dll";

                patchReferenceRow = Core.CreateRow(sourceLineNumbers, "WixPatchRef");
                patchReferenceRow[0] = table;
                patchReferenceRow[1] = key;
            }
        }

        public override void ParseAttribute(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlAttribute attribute, Dictionary<string, string> contextValues)
        {
            ParseAttribute(sourceLineNumbers, parentElement, attribute);
        }

        public override void ParseAttribute(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlAttribute attribute)
        {
            if (!parentElement.NamespaceURI.Equals("http://schemas.microsoft.com/wix/2006/wi"))
            {
                Core.UnexpectedAttribute(sourceLineNumbers, attribute);
                return;
            }

            switch (parentElement.LocalName)
            {
                case "CustomAction":
                    switch (attribute.LocalName)
                    {
                        case "CustomActionData":
                            ParseCustomActionDataAttribute(sourceLineNumbers, parentElement, attribute);
                            break;

                        case "ActionStartText":
                            ParseActionStartTextAttribute(sourceLineNumbers, parentElement, attribute);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attribute);
                            break;
                    }
                    break;

                default:
                    Core.UnexpectedElement(parentElement, attribute);
                    break;
            }
        }

        private void ParseActionStartTextAttribute(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlAttribute attribute)
        {
            string actionStartText = Core.GetAttributeValue(sourceLineNumbers, attribute);
            if (string.IsNullOrEmpty(actionStartText))
            {
                Core.OnMessage(WixErrors.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.LocalName, attribute.LocalName));
                return;
            }

            XmlAttribute idAttrib = parentElement.GetAttributeNode("Id");
            if (idAttrib == null)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.LocalName, "Id"));
                return;
            }
            string caId = Core.GetAttributeIdentifierValue(sourceLineNumbers, idAttrib);
            if (string.IsNullOrEmpty(caId))
            {
                Core.OnMessage(WixErrors.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.LocalName, "Id"));
                return;
            }

            Row row = Core.CreateRow(sourceLineNumbers, "ActionText");
            row[0] = caId;
            row[1] = actionStartText;
            row[2] = "";
        }

        private void ParseCustomActionDataAttribute(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlAttribute attribute)
        {
            string cad = Core.GetAttributeValue(sourceLineNumbers, attribute);
            if (string.IsNullOrEmpty(cad))
            {
                Core.OnMessage(WixErrors.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.LocalName, attribute.LocalName));
            }

            XmlAttribute executeAttrib = parentElement.GetAttributeNode("Execute");
            if (executeAttrib == null)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.LocalName, "Execute"));
                return;
            }
            string execute = Core.GetAttributeValue(sourceLineNumbers, executeAttrib);
            Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType executeType = Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ParseExecuteType(execute);
            switch (executeType)
            {
                case Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.commit:
                case Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.deferred:
                case Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.rollback:
                    break;

                default:
                    // CustomActionData is only relevant for deferred actions
                    Core.OnMessage(WixErrors.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, parentElement.LocalName, "Execute", execute, attribute.LocalName));
                    return;
            }

            XmlAttribute dllEntryAttrib = parentElement.GetAttributeNode("DllEntry");
            if (dllEntryAttrib == null)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.LocalName, "DllEntry"));
            }
            string dllEntry = Core.GetAttributeValue(sourceLineNumbers, dllEntryAttrib);
            if (string.IsNullOrWhiteSpace(dllEntry))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.LocalName, "DllEntry"));
                return;
            }

            XmlAttribute idAttrib = parentElement.GetAttributeNode("Id");
            if (idAttrib == null)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, parentElement.LocalName, "Id"));
            }
            string caId = Core.GetAttributeIdentifierValue(sourceLineNumbers, idAttrib);
            if (string.IsNullOrEmpty(caId))
            {
                Core.OnMessage(WixErrors.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.LocalName, "Id"));
            }

            if (Core.EncounteredError)
            {
                return;
            }

            Row row = Core.CreateRow(sourceLineNumbers, "CustomAction");
            row[0] = $"Set{caId}";
            row[1] = 0x00000030 | 0x00000003; // Set formatted property
            row[2] = caId;
            row[3] = cad;

            Row sequenceRow = Core.CreateRow(sourceLineNumbers, "WixAction");
            sequenceRow[0] = "InstallExecuteSequence";
            sequenceRow[1] = $"Set{caId}";
            sequenceRow[2] = null; // condition
            sequenceRow[4] = caId; // beforeAction
            sequenceRow[5] = null; // afterAction
            sequenceRow[6] = 0; // not overridable

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", caId);

            // Set action text to the SetProperty element as well if it is set for the CA
            XmlAttribute caTextAttrib = parentElement.GetAttributeNode("ActionStartText", attribute.NamespaceURI);
            if (caTextAttrib != null)
            {
                string actionText = Core.GetAttributeValue(sourceLineNumbers, caTextAttrib);
                if (!string.IsNullOrWhiteSpace(actionText))
                {
                    row = Core.CreateRow(sourceLineNumbers, "ActionText");
                    row[0] = $"Set{caId}";
                    row[1] = actionText;
                    row[2] = "";
                }
            }
        }

        private void ParseExecuteCommand(XmlElement element)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = "exc" + Guid.NewGuid().ToString("N");
            ErrorHandling promptOnError = ErrorHandling.fail;
            bool isAsync = false, impersonate = true;
            string command = null, workingFolder = "", condition = null, before = null, after = null;
            Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType executeType = Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.deferred;
            int executeTypeVal = 0x0;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }
                switch (attrib.LocalName)
                {
                    case "Id":
                        id = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;
                    case "ErrorHandling":
                        {
                            string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                            }
                            catch
                            {
                                Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            }
                        }
                        break;
                    case "Wait":
                        isAsync = (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.No);
                        break;
                    case "Impersonate":
                        impersonate = (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                        break;
                    case "Command":
                        command = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;
                    case "WorkingDirectory":
                        workingFolder = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;
                    case "Before":
                        before = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;
                    case "After":
                        after = Core.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                        break;
                    case "Condition":
                        condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;
                    case "Execute":
                        string val = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        if (!Enum.TryParse(val, out executeType))
                        {
                            Core.OnMessage(WixErrors.IllegalAttributeValueWithIllegalList(sourceLineNumbers, element.LocalName, attrib.LocalName, val, "deferred, commit, rollback"));
                            break;
                        }
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(command))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Command"));
            }
            if (string.IsNullOrEmpty(after) == string.IsNullOrEmpty(before))
            {
                Core.OnMessage(WixErrors.NeedSequenceBeforeOrAfter(sourceLineNumbers, element.LocalName));
            }
            switch (executeType)
            {
                case Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.deferred:
                    executeTypeVal = 0x00000400;
                    break;
                case Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.rollback:
                    executeTypeVal = 0x00000400 | 0x00000100;
                    break;
                case Microsoft.Tools.WindowsInstallerXml.Serialize.CustomAction.ExecuteType.commit:
                    executeTypeVal = 0x00000400 | 0x00000200;
                    break;
                default:
                    Core.OnMessage(WixErrors.IllegalAttributeValueWithIllegalList(sourceLineNumbers, element.LocalName, "Execute", executeType.ToString(), "deferred, commit, rollback"));
                    break;
            }

            if (Core.EncounteredError)
            {
                return;
            }

            // Create 3 custom actions:
            // 1. Set PSW_ExecuteCommand property
            // 2. Call PanelSwCustomActions::ExecuteCommand
            // 3. PanelSwCustomActions::CommonDeferred

            // 1. Set PSW_ExecuteCommand property
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "Binary", "PanelSwCustomActions.dll");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "Property", "PSW_ExecuteCommand");
            JObject json = JObject.FromObject(new { Id = id, Command = command, WorkingFolder = workingFolder, Async = isAsync, ErrorHandling = (int)promptOnError });
            string cad = json.ToString();
            cad = Regex.Replace(cad, @"([\[\]\{\}])", @"[\$1]");

            Row PSW_ExecuteCommand = Core.CreateRow(sourceLineNumbers, "PSW_ExecuteCommand");
            PSW_ExecuteCommand[0] = id;

            Row prepareCA = Core.CreateRow(sourceLineNumbers, "CustomAction");
            prepareCA[0] = $"Prepare{id}";
            prepareCA[1] = 0x00000030 | 0x00000003; // Set formatted property
            prepareCA[2] = "PSW_ExecuteCommand";
            prepareCA[3] = cad;

            Row setCadSched = Core.CreateRow(sourceLineNumbers, "WixAction");
            setCadSched[0] = "InstallExecuteSequence";
            setCadSched[1] = $"Prepare{id}";
            setCadSched[2] = condition;
            setCadSched[4] = $"Sched{id}"; // beforeAction
            setCadSched[5] = null; // afterAction
            setCadSched[6] = 0; // not overridable

            // 2. Call PanelSwCustomActions::ExecuteCommand
            Row schedCAD = Core.CreateRow(sourceLineNumbers, "CustomAction");
            schedCAD[0] = $"Sched{id}";
            schedCAD[1] = 0x00000001 | 0x00000000; // MsidbCustomActionTypeDll | MsidbCustomActionTypeBinaryData
            schedCAD[2] = "PanelSwCustomActions.dll";
            schedCAD[3] = "ExecuteCommand";

            Row schedCadSched = Core.CreateRow(sourceLineNumbers, "WixAction");
            schedCadSched[0] = "InstallExecuteSequence";
            schedCadSched[1] = $"Sched{id}";
            schedCadSched[2] = condition; // condition
            schedCadSched[4] = id; // beforeAction
            schedCadSched[5] = null; // afterAction
            schedCadSched[6] = 0; // not overridable

            // 3. PanelSwCustomActions::CommonDeferred
            Row deferredCA = Core.CreateRow(sourceLineNumbers, "CustomAction");
            deferredCA[0] = id;
            deferredCA[1] = 0x00000001/*Dll*/ | 0x00002000 /*Hidden*/ | (impersonate ? 0 : 0x00000800) | executeTypeVal;
            deferredCA[2] = "PanelSwCustomActions.dll";
            deferredCA[3] = "CommonDeferred";

            Row deferredCASched = Core.CreateRow(sourceLineNumbers, "WixAction");
            deferredCASched[0] = "InstallExecuteSequence";
            deferredCASched[1] = id;
            deferredCASched[2] = condition;
            deferredCASched[4] = before;
            deferredCASched[5] = after;
            deferredCASched[6] = 0; // not overridable

            // UI message templates
            Row actionTextRow = Core.CreateRow(sourceLineNumbers, "ActionText");
            actionTextRow[0] = id;
            actionTextRow[1] = "!(loc.ExecOnComponent)";
            actionTextRow[2] = "!(loc.ExecOnComponentTemplate)";
        }

        private void ParseWebsiteConfigElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = "web" + Guid.NewGuid().ToString("N");
            string website = null;
            bool stop = false;
            bool start = false;
            YesNoDefaultType autoStart = YesNoDefaultType.Default;
            ErrorHandling promptOnError = ErrorHandling.fail;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Website":
                        website = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Stop":
                        stop = (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                        break;

                    case "Start":
                        start = (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                        break;

                    case "AutoStart":
                        autoStart = Core.GetAttributeYesNoDefaultValue(sourceLineNumbers, attrib);
                        break;

                    case "ErrorHandling":
                        {
                            string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                            }
                            catch
                            {
                                Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            }
                        }
                        break;

                    case "Order":
                        order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                        if (order < 0)
                        {
                            order += int.MaxValue;
                        }
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(website))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Website"));
            }

            if (Core.EncounteredError)
            {
                return;
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_WebsiteConfigSched");
            Row row = Core.CreateRow(sourceLineNumbers, "PSW_WebsiteConfig");
            row[0] = id;
            row[1] = component;
            row[2] = website;
            row[3] = stop ? 1 : 0;
            row[4] = start ? 1 : 0;
            row[5] = (autoStart == YesNoDefaultType.Yes) ? 1 : (autoStart == YesNoDefaultType.No) ? 0 : -1;
            row[6] = (int)promptOnError;
            row[7] = order;
        }

        private void ParseIsWindowsVersionOrGreater(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string property = null;
            string version = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Version":
                            version = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(version))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Version"));
            }
            if (!Version.TryParse(version, out Version v))
            {
                Core.OnMessage(WixErrors.IllegalVersionValue(sourceLineNumbers, node.LocalName, "Version", version));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "IsWindowsVersionOrGreater");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_IsWindowsVersionOrGreater");
                row[0] = "wmv" + Guid.NewGuid().ToString("N");
                row[1] = property;
                row[2] = version;
            }
        }

        private void ParseMd5Hash(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string property = null;
            string plain = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Plain":
                            plain = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(plain))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Plain"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "Md5Hash");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Md5Hash");
                row[0] = "md5" + Guid.NewGuid().ToString("N");
                row[1] = property;
                row[2] = plain;
            }
        }

        private void ParseToLowerCase(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string property = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_ToLowerCase");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ToLowerCase");
                row[0] = property;
            }
        }

        private void ParseJsonJpathSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "_" + Guid.NewGuid().ToString("N");
            string property = null;
            string expression = null;
            string file = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "JPath":
                            expression = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            file = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "JPath"));
            }
            if (string.IsNullOrEmpty(file))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "FilePath"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "JsonJpathSearch");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_JsonJpathSearch");
                row[0] = id;
                row[1] = property;
                row[2] = expression;
                row[3] = file;
            }
        }

        private enum JsonFormatting
        {
            Raw,
            String,
            Boolean
        }

        private void ParseJsonJPathElement(XmlElement node, string component_, string file_)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "jpt" + Guid.NewGuid().ToString("N");
            string jpath = null;
            string value = null;
            string filePath = null;
            JsonFormatting jsonFormatting = JsonFormatting.Raw;
            ErrorHandling promptOnError = ErrorHandling.fail;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "JPath":
                            jpath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FilePath":
                            filePath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Value":
                            value = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Formatting":
                            string formatting = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (!Enum.TryParse(formatting, true, out jsonFormatting))
                            {
                                Core.OnMessage(WixErrors.IllegalAttributeValueWithLegalList(sourceLineNumbers, node.LocalName, attrib.LocalName, formatting, $"{JsonFormatting.Raw}, {JsonFormatting.String}"));
                            }
                            break;

                        case "ErrorHandling":
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                                }
                            }
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file_) == string.IsNullOrEmpty(filePath))
            {
                Core.OnMessage(WixErrors.IllegalAttributeWhenNested(sourceLineNumbers, node.LocalName, "FilePath", "File"));
            }
            if (string.IsNullOrEmpty(jpath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "JPath"));
            }
            if (string.IsNullOrEmpty(value))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Value"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "JsonJpathSched");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_JsonJPath");
                row[0] = id;
                row[1] = component_;
                row[2] = filePath;
                row[3] = file_;
                row[4] = jpath;
                row[5] = value;
                row[6] = (int)jsonFormatting;
                row[7] = (int)promptOnError;
            }
        }

        private void ParseDiskSpaceElement(XmlElement element, string directory)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "DiskSpace");
            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_DiskSpace");
                row[0] = directory;
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
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_SetPropertyFromPipe");
                row[0] = id;
                row[1] = pipe;
                row[2] = timeout;
            }
        }

        private void ParseCreateSelfSignCertificateElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string password = null;
            string x500 = null;
            string subjectAltName = null;
            int expiry = 0;
            bool deleteOnCommit = true;

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

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(x500))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "X500"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CreateSelfSignCertificate");
            if (!Core.EncounteredError)
            {
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

        private void ParseBackupAndRestoreElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string filepath = null;
            BackupAndRestore_deferred_Schedule restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
            DeletePathFlags flags = 0;

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
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.IgnoreMissing;
                            }
                            break;
                        case "ignoreerrors":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
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
                id = "bnr" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Path"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "BackupAndRestore");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "Property", restoreSchedule.ToString());

            if (!Core.EncounteredError)
            {
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

        private void ParseInstallUtilElement(XmlNode node, string file)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            InstallUtil_Bitness bitness = InstallUtil_Bitness.asComponent;

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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "File", "Id"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_InstallUtilSched");

            if (!Core.EncounteredError)
            {
                // Ensure sub-table exists for queries to succeed even if no sub-entries exist.
                Core.EnsureTable(sourceLineNumbers, "PSW_InstallUtil_Arg");
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
                    Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, child.LocalName, "Id"));
                    continue;
                }
                if (string.IsNullOrEmpty(value))
                {
                    Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, child.LocalName, "Value"));
                    continue;
                }

                Row row = Core.CreateRow(sourceLineNumbers, "PSW_InstallUtil_Arg");
                row[0] = file;
                row[1] = argId;
                row[2] = value;
            }
        }

        private void ParseForceVersionElement(XmlNode node, string file)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string version = "65535.65535.65535.65535";

            if (node.LocalName.Equals("AlwaysOverwriteFile"))
            {
                Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, node.LocalName, "ForceVersion"));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Version":
                            version = Core.GetAttributeVersionValue(sourceLineNumbers, attrib, true);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "File", "Id"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ForceVersion");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ForceVersion");
                row[0] = file;
                row[1] = version;
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

        enum ErrorHandling
        {
            fail = 0,
            ignore = 1,
            prompt = 2,
            promptAlways = 3, // Prompt even if exit code and stdout were ok
        }

        private void ParseTopShelfElement(XmlNode node, string file)
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
            ErrorHandling promptOnError = ErrorHandling.fail;

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
                                    promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "File", "Id"));
            }
            if (string.IsNullOrEmpty(userName) != (account != TopShelf_Account.custom))
            {
                Core.OnMessage(WixErrors.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, "TopShelf", "Account", "custom", "UserName"));
            }

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

        // Definition must match ExeOnComponent.cpp
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

            // Not waiting
            ASync = 2 * AfterStartServices,

            // Impersonate
            Impersonate = 2 * ASync,
        }

        private int GetLineNumber(SourceLineNumberCollection sourceLineNumbers)
        {
            int order = 0;
            if ((sourceLineNumbers != null) && (sourceLineNumbers.Count > 0))
            {
                foreach (SourceLineNumber line in sourceLineNumbers)
                {
                    if (line.HasLineNumber)
                    {
                        order = line.LineNumber;
                    }
                }
            }
            return order;
        }

        [Flags]
        private enum SqlExecOn
        {
            None = 0,
            Install = 1,
            InstallRollback = Install * 2,
            Uninstall = InstallRollback * 2,
            UninstallRollback = Uninstall * 2,
            Reinstall = UninstallRollback * 2,
            ReinstallRollback = Reinstall * 2,
        }

        private void ParseSqlScriptElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = "sql" + Guid.NewGuid().ToString("N");
            string binary = null;
            string driver = null;
            string server = null;
            string connectionString = null;
            string instance = null;
            string database = null;
            string username = null;
            string password = null;
            string port = null;
            string encrypted = null;
            ErrorHandling? errorHandling = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            SqlExecOn sqlExecOn = SqlExecOn.None;
            YesNoType aye;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Id":
                        id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "BinaryKey":
                        binary = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ConnectionString":
                        connectionString = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Driver":
                        driver = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Server":
                        server = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Instance":
                        instance = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Port":
                        port = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Encrypt":
                        encrypted = Core.GetAttributeValue(sourceLineNumbers, attrib);
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

                    case "Order":
                        order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                        if (order < 0)
                        {
                            order += int.MaxValue;
                        }
                        break;

                    case "ErrorHandling":
                        string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        try
                        {
                            errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                        }
                        catch
                        {
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        }
                        break;

                    case "OnInstall":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.Install;
                        }
                        break;

                    case "OnInstallRollback":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.InstallRollback;
                        }
                        break;

                    case "OnReinstall":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.Reinstall;
                        }
                        break;

                    case "OnReinstallRollback":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.ReinstallRollback;
                        }
                        break;

                    case "OnUninstall":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.Uninstall;
                        }
                        break;

                    case "OnUninstallRollback":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            sqlExecOn |= SqlExecOn.UninstallRollback;
                        }
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (!string.IsNullOrEmpty(connectionString) &&
                (!string.IsNullOrEmpty(server) || !string.IsNullOrEmpty(instance) || !string.IsNullOrEmpty(database) || !string.IsNullOrEmpty(port) || !string.IsNullOrEmpty(encrypted) || !string.IsNullOrEmpty(password) || !string.IsNullOrEmpty(username)))
            {
                Core.OnMessage(WixErrors.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.LocalName, "ConnectionString", "any other"));
            }
            if (string.IsNullOrEmpty(server) && string.IsNullOrEmpty(connectionString))
            {
                Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, element.LocalName, "Server", "ConnectionString"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(binary))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "BinaryKey"));
            }
            if (sqlExecOn == SqlExecOn.None)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "OnXXX"));
            }

            // ExitCode mapping
            foreach (XmlNode child in element.ChildNodes)
            {
                SourceLineNumberCollection repLines = Preprocessor.GetSourceLineNumbers(child);
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch (child.LocalName)
                    {
                        case "Replace":
                            {
                                string from = null;
                                string to = "";
                                int repOrder = 1000000000 + GetLineNumber(repLines);
                                foreach (XmlAttribute a in child.Attributes)
                                {
                                    if ((0 != a.NamespaceURI.Length) && (a.NamespaceURI != schema.TargetNamespace))
                                    {
                                        continue;
                                    }

                                    switch (a.LocalName)
                                    {
                                        case "Text":
                                            from = Core.GetAttributeValue(repLines, a);
                                            break;

                                        case "Replacement":
                                            to = Core.GetAttributeValue(repLines, a, true);
                                            break;

                                        case "Order":
                                            repOrder = Core.GetAttributeIntegerValue(repLines, a, -1000000000, 1000000000);
                                            if (repOrder < 0)
                                            {
                                                repOrder += int.MaxValue;
                                            }
                                            break;
                                    }
                                }

                                Row row = Core.CreateRow(repLines, "PSW_SqlScript_Replacements");
                                row[0] = id;
                                row[1] = from;
                                row[2] = to;
                                row[3] = repOrder;
                            }
                            break;

                        default:
                            Core.UnexpectedElement(element, child);
                            break;
                    }
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_SqlScript");

            if (!Core.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                Core.EnsureTable(sourceLineNumbers, "PSW_SqlScript_Replacements");
                Core.EnsureTable(sourceLineNumbers, "PSW_SqlScript");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_SqlScript");
                row[0] = id;
                row[1] = component;
                row[2] = binary;
                row[3] = server;
                row[4] = instance;
                row[5] = port;
                row[6] = encrypted;
                row[7] = database;
                row[8] = username;
                row[9] = password;
                row[10] = (int)sqlExecOn;
                row[11] = (int)(errorHandling ?? ErrorHandling.fail);
                row[12] = order;
                row[13] = connectionString;
                row[14] = driver;
            }
        }

        private void ParseXslTransform(XmlElement element, string component, string file)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = "xsl" + Guid.NewGuid().ToString("N");
            string filePath = null;
            string binary = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType on = Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType.install;

            foreach (XmlAttribute attrib in element.Attributes)
            {
                if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                {
                    continue;
                }

                switch (attrib.LocalName)
                {
                    case "Id":
                        id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "BinaryKey":
                        binary = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "FilePath":
                        filePath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "On":
                        on = Core.GetAttributeInstallUninstallValue(sourceLineNumbers, attrib);
                        break;

                    case "Order":
                        order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                        if (order < 0)
                        {
                            order += int.MaxValue;
                        }
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(file) == string.IsNullOrEmpty(filePath))
            {
                Core.OnMessage(WixErrors.ExpectedAttributeInElementOrParent(sourceLineNumbers, element.LocalName, "FilePath", "File"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(binary))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "BinaryKey"));
            }
            if (on == Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType.NotSet)
            {
                on = Microsoft.Tools.WindowsInstallerXml.Serialize.InstallUninstallType.install;
            }

            // Text replacements in XSL
            foreach (XmlNode child in element.ChildNodes)
            {
                SourceLineNumberCollection repLines = Preprocessor.GetSourceLineNumbers(child);
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch (child.LocalName)
                    {
                        case "Replace":
                            {
                                string from = null;
                                string to = "";
                                int repOrder = 1000000000 + GetLineNumber(repLines);
                                foreach (XmlAttribute a in child.Attributes)
                                {
                                    if ((0 != a.NamespaceURI.Length) && (a.NamespaceURI != schema.TargetNamespace))
                                    {
                                        continue;
                                    }

                                    switch (a.LocalName)
                                    {
                                        case "Text":
                                            from = Core.GetAttributeValue(repLines, a);
                                            break;

                                        case "Replacement":
                                            to = Core.GetAttributeValue(repLines, a, true);
                                            break;

                                        case "Order":
                                            repOrder = Core.GetAttributeIntegerValue(repLines, a, -1000000000, 1000000000);
                                            if (repOrder < 0)
                                            {
                                                repOrder += int.MaxValue;
                                            }
                                            break;
                                    }
                                }

                                Row row = Core.CreateRow(repLines, "PSW_XslTransform_Replacements");
                                row[0] = id;
                                row[1] = from;
                                row[2] = to;
                                row[3] = repOrder;
                            }
                            break;

                        default:
                            Core.UnexpectedElement(element, child);
                            break;
                    }
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_XslTransform");

            if (!Core.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                Core.EnsureTable(sourceLineNumbers, "PSW_XslTransform_Replacements");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_XslTransform");
                int i = 0;
                row[i++] = id;
                row[i++] = file;
                row[i++] = component;
                row[i++] = filePath;
                row[i++] = binary;
                row[i++] = order;
                row[i++] = (int)on;
            }
        }

        private void ParseExecOnComponentElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string binary = null;
            string command = null;
            string workDir = null;
            ExecOnComponentFlags flags = ExecOnComponentFlags.None;
            ErrorHandling? errorHandling = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            YesNoType aye;
            string user = null;

            if (element.HasAttribute("IgnoreExitCode") && element.HasAttribute("ErrorHandling"))
            {
                Core.OnMessage(WixErrors.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.LocalName, "IgnoreExitCode", "ErrorHandling"));
                return;
            }

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

                    case "BinaryKey":
                        binary = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "WorkingDirectory":
                        workDir = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Order":
                        order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                        if (order < 0)
                        {
                            order += int.MaxValue;
                        }
                        break;

                    case "Impersonate":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.Impersonate;
                        }
                        break;

                    case "User":
                        user = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "IgnoreExitCode":
                        Core.OnMessage(WixWarnings.DeprecatedAttribute(sourceLineNumbers, element.LocalName, attrib.LocalName));
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            errorHandling = ErrorHandling.ignore;
                        }
                        break;

                    case "ErrorHandling":
                        string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        try
                        {
                            errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                        }
                        catch
                        {
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        }

                        if (element.HasAttribute("IgnoreExitCode"))
                        {
                            Core.OnMessage(WixErrors.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.LocalName, attrib.LocalName, "IgnoreExitCode"));
                        }
                        break;

                    case "Wait":
                        aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.No)
                        {
                            flags |= ExecOnComponentFlags.ASync;
                            if (errorHandling == null)
                            {
                                errorHandling = ErrorHandling.ignore; // Really isn't checked on async, but just to be on the safe side.
                            }
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "exc" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(command))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "Command"));
            }
            if ((flags & ExecOnComponentFlags.AnyAction) == ExecOnComponentFlags.None)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "OnXXX"));
            }
            if ((flags & ExecOnComponentFlags.AnyTiming) == ExecOnComponentFlags.None)
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "BeforeXXX or AfterXXX"));
            }
            if (((flags & ExecOnComponentFlags.ASync) == ExecOnComponentFlags.ASync) && (errorHandling != ErrorHandling.ignore))
            {
                Core.OnMessage(WixErrors.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, element.LocalName, "Wait", "no", "ErrorHandling"));
            }

            // ExitCode mapping
            foreach (XmlNode child in element.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    switch (child.LocalName)
                    {
                        case "ExitCode":
                            {
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
                            }
                            break;

                        case "ConsoleOutput":
                            {
                                ErrorHandling stdoutHandling = ErrorHandling.fail;
                                string regex = null;
                                string prompt = null;
                                bool onMatch = true;

                                foreach (XmlAttribute a in child.Attributes)
                                {
                                    if ((0 != a.NamespaceURI.Length) && (a.NamespaceURI != schema.TargetNamespace))
                                    {
                                        continue;
                                    }

                                    switch (a.LocalName)
                                    {
                                        case "Expression":
                                            regex = Core.GetAttributeValue(sourceLineNumbers, a);
                                            break;

                                        case "BehaviorOnMatch":
                                            onMatch = (Core.GetAttributeYesNoValue(sourceLineNumbers, a) == YesNoType.Yes);
                                            break;

                                        case "PromptText":
                                            prompt = Core.GetAttributeValue(sourceLineNumbers, a);
                                            break;

                                        case "Behavior":
                                            try
                                            {
                                                stdoutHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), Core.GetAttributeValue(sourceLineNumbers, a));
                                            }
                                            catch
                                            {
                                                Core.UnexpectedAttribute(sourceLineNumbers, a);
                                            }
                                            break;
                                    }
                                }

                                if ((stdoutHandling == ErrorHandling.prompt) == string.IsNullOrEmpty(prompt))
                                {
                                    Core.OnMessage(WixErrors.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, child.LocalName, "Behavior", "prompt", "PromptText"));
                                }
                                if (string.IsNullOrEmpty(regex))
                                {
                                    Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, child.LocalName, "Expression"));
                                }

                                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ExecOn_ConsoleOutput");
                                row[0] = "std" + Guid.NewGuid().ToString("N");
                                row[1] = id;
                                row[2] = regex;
                                row[3] = onMatch ? 1 : 0;
                                row[4] = (int)stdoutHandling;
                                row[5] = prompt;
                            }
                            break;


                        case "Environment":
                            {
                                string name = null, value = null;

                                foreach (XmlAttribute a in child.Attributes)
                                {
                                    if ((0 != a.NamespaceURI.Length) && (a.NamespaceURI != schema.TargetNamespace))
                                    {
                                        Core.UnsupportedExtensionAttribute(sourceLineNumbers, a);
                                    }

                                    switch (a.LocalName)
                                    {
                                        case "Value":
                                            value = Core.GetAttributeValue(sourceLineNumbers, a);
                                            break;

                                        case "Name":
                                            name = Core.GetAttributeValue(sourceLineNumbers, a);
                                            break;

                                        default:
                                            Core.UnsupportedExtensionAttribute(sourceLineNumbers, a);
                                            break;
                                    }
                                }

                                if (string.IsNullOrEmpty(name) || string.IsNullOrEmpty(value))
                                {
                                    Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, child.LocalName, "Name", "Value"));
                                    break;
                                }

                                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ExecOnComponent_Environment");
                                row[0] = id;
                                row[1] = name;
                                row[2] = value;
                            }
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
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                Core.EnsureTable(sourceLineNumbers, "PSW_ExecOnComponent_ExitCode");
                Core.EnsureTable(sourceLineNumbers, "PSW_ExecOn_ConsoleOutput");
                Core.EnsureTable(sourceLineNumbers, "PSW_ExecOnComponent_Environment");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ExecOnComponent");
                row[0] = id;
                row[1] = component;
                row[2] = binary;
                row[3] = command;
                row[4] = workDir;
                row[5] = (int)flags;
                row[6] = (int)(errorHandling ?? ErrorHandling.fail);
                row[7] = order;
                row[8] = user;
            }
        }

        enum ServiceStart : int
        {
            boot = 0,
            unchanged = -1,
            auto = 2,
            demand = 3,
            disabled = 4,
            system = 1,

            autoDelayed = 5
        }

        private void ParseServiceConfigElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string service = null;
            string commandLine = null;
            string account = null;
            string password = null;
            string loadOrderGroup = null;
            ServiceStart start = ServiceStart.unchanged;
            ErrorHandling errorHandling = ErrorHandling.fail;

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

                    case "CommandLine":
                        commandLine = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Account":
                        account = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Password":
                        password = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Start":
                        start = (ServiceStart)Enum.Parse(typeof(ServiceStart), Core.GetAttributeValue(sourceLineNumbers, attrib));
                        break;

                    case "LoadOrderGroup":
                        loadOrderGroup = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ErrorHandling":
                        {
                            string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                            }
                            catch
                            {
                                Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            }
                        }
                        break;

                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(service))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.LocalName, "ServiceName"));
            }
            if (string.IsNullOrEmpty(account) && !string.IsNullOrEmpty(password))
            {
                Core.OnMessage(WixErrors.ExpectedAttributesWithOtherAttribute(sourceLineNumbers, element.LocalName, "Password", "Account"));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "svc" + Guid.NewGuid().ToString("N");
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_ServiceConfig");

            if (!Core.EncounteredError)
            {
                // Ensure sub-table exists for queries to succeed even if no sub-entries exist.
                Core.EnsureTable(sourceLineNumbers, "PSW_ServiceConfig_Dependency");
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ServiceConfig");
                row[0] = id;
                row[1] = component;
                row[2] = service;
                row[3] = commandLine;
                row[4] = account;
                row[5] = password;
                row[6] = (start == ServiceStart.autoDelayed) ? (int)ServiceStart.auto : (int)start;
                row[7] = (start == ServiceStart.autoDelayed) ? 1 : (start == ServiceStart.auto) ? 0 : -1;
                row[8] = loadOrderGroup;
                row[9] = (int)errorHandling;
            }

            foreach (XmlNode child in element.ChildNodes)
            {
                if (child.NamespaceURI != element.NamespaceURI)
                {
                    continue;
                }

                if (!child.LocalName.Equals("Dependency"))
                {
                    Core.UnsupportedExtensionElement(element, child);
                    continue;
                }

                foreach (XmlAttribute attrib in child.Attributes)
                {
                    if ((0 != attrib.NamespaceURI.Length) && (attrib.NamespaceURI != schema.TargetNamespace))
                    {
                        Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                    }
                    string depService = null;
                    string group = null;

                    switch (attrib.LocalName)
                    {
                        case "Service":
                            depService = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Group":
                            group = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }

                    if (string.IsNullOrEmpty(depService) && string.IsNullOrEmpty(group))
                    {
                        Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, child.LocalName, "Service", "Group"));
                    }

                    if (!Core.EncounteredError)
                    {
                        Row row = Core.CreateRow(sourceLineNumbers, "PSW_ServiceConfig_Dependency");
                        row[0] = id;
                        row[1] = depService;
                        row[2] = group;
                    }
                }
            }
        }

        private void ParseDismElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = null;
            string features = null;
            string exclude = null;
            string package = null;
            string unwanted = null;
            ErrorHandling promptOnError = ErrorHandling.fail;
            int cost = 20971520; // 20 MB.
            bool enableAll = true;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);
            bool forceRemove = false;

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

                    case "PackagePath":
                        package = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Id":
                        id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Cost":
                        cost = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, int.MaxValue);
                        break;

                    case "ErrorHandling":
                        {
                            string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                            }
                            catch
                            {
                                Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            }
                        }
                        break;

                    case "EnableAll":
                        enableAll = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) != YesNoType.No;
                        break;

                    case "RemoveFeature":
                        unwanted = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ForceRemove":
                        forceRemove = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes;
                        break;

                    case "Order":
                        order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                        if (order < 0)
                        {
                            order += int.MaxValue;
                        }
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(features) && string.IsNullOrEmpty(package))
            {
                Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, element.LocalName, "EnableFeature", "PackagePath"));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "dsm" + Guid.NewGuid().ToString("N");
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "DismSched");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Dism");
                row[0] = id;
                row[1] = component;
                row[2] = features;
                row[3] = exclude;
                row[4] = package;
                row[5] = cost;
                row[6] = (int)promptOnError;
                row[7] = enableAll ? 1 : 0;
                row[8] = unwanted;
                row[9] = forceRemove ? 1 : 0;
                row[10] = order;
            }
        }

        private void ParseTaskSchedulerElement(XmlElement element, string component)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(element);
            string id = "tsk" + Guid.NewGuid().ToString("N");
            string taskXml = null;
            string taskName = null;
            string user = null;
            string password = null;

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

                    case "User":
                        user = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Password":
                        password = Core.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "XmlFile":
                        taskXml = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                    if (!string.IsNullOrWhiteSpace(taskXml))
                    {
                        Core.OnMessage(WixErrors.IllegalAttributeWithInnerText(sourceLineNumbers, element.LocalName, "XmlFile"));
                    }
                    taskXml = child.Value;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrWhiteSpace(taskXml))
            {
                Core.OnMessage(WixErrors.ExpectedAttributeOrElement(sourceLineNumbers, element.LocalName, "XmlFile", "Inner text or CDATA"));
            }
            if (string.IsNullOrEmpty(taskName))
            {
                Core.OnMessage(WixErrors.ExpectedElement(sourceLineNumbers, element.LocalName, "TaskName"));
            }
            if (string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(password))
            {
                Core.OnMessage(WixErrors.ExpectedAttributesWithOtherAttribute(sourceLineNumbers, element.LocalName, "User", "Password"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "TaskScheduler");

            if (!Core.EncounteredError)
            {
                taskXml = taskXml.Trim();
                taskXml = taskXml.Replace("\r", "");
                taskXml = taskXml.Replace("\n", "");
                taskXml = taskXml.Replace(Environment.NewLine, "");

                Row row = Core.CreateRow(sourceLineNumbers, "PSW_TaskScheduler");
                row[0] = id;
                row[1] = taskName;
                row[2] = component;
                row[3] = taskXml;
                row[4] = user;
                row[5] = password;
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
            string productCode = null;
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
                        case "productcode":
                            productCode = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                        case "condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                id = "uni" + Guid.NewGuid().ToString("N");
            }

            if (string.IsNullOrEmpty(name))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Name"));
            }

            if (string.IsNullOrEmpty(data))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Data"));
            }

            if (string.IsNullOrEmpty(datatype))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "DataType"));
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
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_Immediate");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_deferred");
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_rollback");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_CustomUninstallKey");
                row[0] = id;
                row[1] = productCode;
                row[2] = name;
                row[3] = data;
                row[4] = datatype;
                row[5] = (int)attributes;
                row[6] = condition;
            }
        }

        private enum ReadIniValuesAttributes
        {
            None = 0,
            IgnoreErrors = 1
        }

        private void ParseReadIniValuesElement(XmlNode node, XmlElement parent)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string DestProperty = null;
            string FilePath = null;
            string Section = null;
            string Key = null;
            YesNoType IgnoreErrors = YesNoType.No;
            string condition = null;

            if ((parent != null) && parent.LocalName.Equals("Property"))
            {
                DestProperty = parent.Attributes["Id"]?.Value;
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "DestProperty":
                            if (!string.IsNullOrEmpty(DestProperty))
                            {
                                Core.OnMessage(WixErrors.ExpectedAttributeInElementOrParent(sourceLineNumbers, node.LocalName, attrib.LocalName, parent.LocalName));
                            }
                            DestProperty = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            FilePath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Section":
                            Section = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Key":
                            Key = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreErrors":
                            IgnoreErrors = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                id = "ini" + Guid.NewGuid().ToString("N");
            }

            if (string.IsNullOrEmpty(DestProperty))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "DestProperty"));
            }
            if (!DestProperty.ToUpper().Equals(DestProperty))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", DestProperty));
            }

            if (string.IsNullOrEmpty(FilePath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "FilePath"));
            }

            if (string.IsNullOrEmpty(Key))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Key"));
            }

            if (string.IsNullOrEmpty(Section))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Section"));
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
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ReadIniValues");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ReadIniValues");
                row[0] = id;
                row[1] = FilePath;
                row[2] = Section;
                row[3] = Key;
                row[4] = DestProperty;
                row[5] = (IgnoreErrors == YesNoType.Yes) ? 1 : 0;
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
                                Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, node.LocalName, "Area", Core.GetAttributeValue(sourceLineNumbers, attrib)));
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
                id = "reg" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(key))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Key"));
            }
            if (string.IsNullOrEmpty(root))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Root"));
            }
            if (string.IsNullOrEmpty(name))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Name"));
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

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "RemoveRegistryValue_Immediate");

            if (!Core.EncounteredError)
            {
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
            string friendlyName = null;
            string issuer = null;
            string serial = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "CertName":
                            certName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FriendlyName":
                            friendlyName = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }

            // At least CertName OR (Issuer AND Serial) OR FriendlyName
            if (string.IsNullOrEmpty(issuer) != string.IsNullOrEmpty(serial))
            {
                Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, node.LocalName, "Issuer", "SerialNumber"));
            }
            if (string.IsNullOrEmpty(certName) && string.IsNullOrEmpty(issuer) && string.IsNullOrEmpty(friendlyName))
            {
                Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, node.LocalName, "Issuer", "SerialNumber", "CertName", "FriendlyName"));
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
                Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CertificateHashSearch");

                Row row = Core.CreateRow(sourceLineNumbers, "PSW_CertificateHashSearch");
                row[0] = property;
                row[1] = certName;
                row[2] = friendlyName;
                row[3] = issuer;
                row[4] = serial;
            }
        }

        private void ParseEvaluateElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "_" + Guid.NewGuid().ToString("N");
            string property = null;
            string expression = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

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
                            order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Expression"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (child.NamespaceURI == schema.TargetNamespace)
                {
                    Core.UnexpectedElement(node, child);
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "EvaluateExpression");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_EvaluateExpression");
                row[0] = id;
                row[1] = property;
                row[2] = expression;
                row[3] = order;
            }
        }

        private void ParsePathSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "pth" + Guid.NewGuid().ToString("N"); ;
            string file = null;
            string property = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
                return;
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "FileName":
                            file = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
                return;
            }
            if (string.IsNullOrEmpty(file))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "FileName"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_PathSearch");
            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_PathSearch");
                row[0] = id;
                row[1] = property;
                row[2] = file;
            }
        }

        private void ParseVersionCompareElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "ver" + Guid.NewGuid().ToString("N"); ;
            string version1 = null;
            string version2 = null;
            string property = null;

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
                return;
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Version1":
                            version1 = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Version2":
                            version2 = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
                return;
            }
            if (string.IsNullOrEmpty(version1))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Version1"));
            }
            if (string.IsNullOrEmpty(version2))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Version2"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_VersionCompare");
            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_VersionCompare");
                row[0] = id;
                row[1] = property;
                row[2] = version1;
                row[3] = version2;
            }
        }

        private void ParseAccountSidSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "sid" + Guid.NewGuid().ToString("N"); ;
            string systemName = null;
            string accountName = null;
            string property = null;
            string condition = "";

            if (node.ParentNode.LocalName != "Property")
            {
                Core.UnexpectedElement(node.ParentNode, node);
                return;
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "AccountName":
                            accountName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "SystemName":
                            systemName = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
                return;
            }
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }
            if (string.IsNullOrEmpty(accountName))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "AccountName"));
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
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "AccountSidSearch");
            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_AccountSidSearch");
                row[0] = id;
                row[1] = property;
                row[2] = systemName;
                row[3] = accountName;
                row[4] = condition;
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
            string property;
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
                                Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, node.LocalName, "Match", Core.GetAttributeValue(sourceLineNumbers, attrib)));
                            }
                            break;
                        case "condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.LocalName, "Id"));
            }
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }
            if (string.IsNullOrEmpty(id))
            {
                id = "xms" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(filePath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "FilePath"));
            }
            if (string.IsNullOrEmpty(xpath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "XPath"));
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
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "XmlSearch");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_XmlSearch");
                row[0] = id;
                row[1] = property;
                row[2] = filePath;
                row[3] = xpath;
                row[4] = lang;
                row[5] = namespaces;
                row[6] = (int)match;
                row[7] = condition;
            }
        }

        private void ParseSqlSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "sql" + Guid.NewGuid().ToString("N");
            string property = null;
            string server = null;
            string connectionString = null;
            string instance = null;
            string database = null;
            string username = null;
            string password = null;
            string query = null;
            string condition = null;
            string port = null;
            string encrypted = null;
            ErrorHandling errorHandling = ErrorHandling.fail;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (!node.ParentNode.LocalName.Equals("Property"))
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (string.IsNullOrWhiteSpace(property))
            {
                Core.OnMessage(WixErrors.ParentElementAttributeRequired(sourceLineNumbers, node.ParentNode.LocalName, "Id", node.LocalName));
            }
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "ConnectionString":
                            connectionString = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
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
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;
                        case "Port":
                            port = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Encrypt":
                            encrypted = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ErrorHandling":
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    errorHandling = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
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

            if (!string.IsNullOrEmpty(connectionString) &&
                (!string.IsNullOrEmpty(server) || !string.IsNullOrEmpty(instance) || !string.IsNullOrEmpty(database) || !string.IsNullOrEmpty(port) || !string.IsNullOrEmpty(encrypted) || !string.IsNullOrEmpty(password) || !string.IsNullOrEmpty(username)))
            {
                Core.OnMessage(WixErrors.IllegalAttributeWithOtherAttribute(sourceLineNumbers, node.LocalName, "ConnectionString", "any other"));
            }

            if (string.IsNullOrEmpty(server) && string.IsNullOrEmpty(connectionString))
            {
                Core.OnMessage(WixErrors.ExpectedAttributes(sourceLineNumbers, node.LocalName, "Server", "ConnectionString"));
            }
            if (string.IsNullOrEmpty(query))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Query"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "SqlSearch");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_SqlSearch");
                int i = 0;
                row[i++] = id;
                row[i++] = property;
                row[i++] = server;
                row[i++] = instance;
                row[i++] = port;
                row[i++] = encrypted;
                row[i++] = database;
                row[i++] = username;
                row[i++] = password;
                row[i++] = query;
                row[i++] = condition;
                row[i++] = order;
                row[i++] = (int)errorHandling;
                row[i++] = connectionString;
            }
        }

        private void ParseWmiSearchElement(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "wmi" + Guid.NewGuid().ToString("N");
            string property = null;
            string nmspace = null;
            string query = null;
            string resultProp = null;
            string condition = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (!node.ParentNode.LocalName.Equals("Property"))
            {
                Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;
            if (string.IsNullOrWhiteSpace(property))
            {
                Core.OnMessage(WixErrors.ParentElementAttributeRequired(sourceLineNumbers, node.ParentNode.LocalName, "Id", node.LocalName));
            }
            if (!property.ToUpper().Equals(property))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Namespace":
                            nmspace = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ResultProperty":
                            resultProp = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
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

            if (string.IsNullOrEmpty(query))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Query"));
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "WmiSearch");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_WmiSearch");
                int i = 0;
                row[i++] = id;
                row[i++] = property;
                row[i++] = condition;
                row[i++] = nmspace;
                row[i++] = query;
                row[i++] = resultProp;
                row[i++] = order;
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
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "onstart":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "onfailure":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;
                        case "secure":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
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
                id = "tlm" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(url))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Url"));
            }
            if (string.IsNullOrEmpty(method))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Method"));
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

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "Telemetry");

            if (!Core.EncounteredError)
            {
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
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                wait = 1;
                            }
                            break;
                        case "show":
                            show = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 15);
                            break;

                        case "oncommit":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "onexecute":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "onrollback":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
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
                id = "shl" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(target))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Target"));
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

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ShellExecute_Immediate");

            if (!Core.EncounteredError)
            {
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

        private void ParseMsiSqlQuery(XmlNode node, XmlNode parent)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string query = null;
            string condition = null;
            string property = null;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
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

            if ((parent != null) && parent.LocalName.Equals("Property"))
            {
                property = parent.Attributes["Id"]?.Value;
                if (!property.ToUpper().Equals(property))
                {
                    Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                id = "msq" + Guid.NewGuid().ToString("N");
            }

            if (string.IsNullOrEmpty(query))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Query"));
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
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "MsiSqlQuery");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_MsiSqlQuery");
                row[0] = id;
                row[1] = property;
                row[2] = query;
                row[3] = condition;
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
            string filepath = null;
            string input = null;
            string regex = null;
            string replacement = null;
            string prop = null;
            int flags = 0;
            string condition = null;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            if (node.ParentNode.LocalName == "Property")
            {
                prop = node.ParentNode.Attributes["Id"].Value;
            }

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            filepath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Input":
                            input = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Expression":
                            regex = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Replacement":
                            replacement = Core.GetAttributeValue(sourceLineNumbers, attrib, true);
                            flags |= (int)RegexSearchFlags.Replace;
                            break;
                        case "DstProperty":
                            if (!string.IsNullOrEmpty(prop))
                            {
                                Core.OnMessage(WixErrors.ExpectedAttributeInElementOrParent(sourceLineNumbers, node.LocalName, attrib.LocalName, node.ParentNode.LocalName));
                            }
                            prop = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreCase":
                            flags |= (int)RegexMatchFlags.IgnoreCare << 2;
                            break;
                        case "Extended":
                            flags |= (int)RegexMatchFlags.Extended << 2;
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
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
                id = "rgx" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(regex))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Expression"));
            }
            if (string.IsNullOrEmpty(prop))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "DstProperty"));
            }
            if (!prop.ToUpper().Equals(prop))
            {
                Core.OnMessage(WixErrors.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", prop));
            }
            if (string.IsNullOrEmpty(input) == string.IsNullOrEmpty(filepath))
            {
                Core.OnMessage(WixErrors.IllegalAttributeWithOtherAttribute(sourceLineNumbers, node.LocalName, "Input", "FilePath"));
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
                // Condition can be specified on attribute 'Condition' in which case embedded text may be the property default value.
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "RegularExpression");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_RegularExpression");
                row[0] = id;
                row[1] = filepath;
                row[2] = input;
                row[3] = regex;
                row[4] = replacement;
                row[5] = prop;
                row[6] = flags;
                row[7] = condition;
                row[8] = order;
            }
        }

        private void ParseRestartLocalResources(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = "rst" + Guid.NewGuid().ToString("N");
            string path = null;
            string condition = null;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Path":
                            path = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(path))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Path"));
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
                }
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "RestartLocalResources");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_RestartLocalResources");
                row[0] = id;
                row[1] = path;
                row[2] = condition;
            }
        }

        private enum FileEncoding
        {
            AutoDetect,
            MultiByte,
            Unicode,
            ReverseUnicode
        };

        private void ParseFileRegex(XmlNode node, string component, string fileId)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string condition = null;
            string regex = null;
            string replacement = null;
            FileEncoding encoding = FileEncoding.AutoDetect;
            bool ignoreCase = false;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            filepath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Regex":
                            regex = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Replacement":
                            replacement = Core.GetAttributeValue(sourceLineNumbers, attrib, true);
                            break;
                        case "IgnoreCase":
                            ignoreCase = true;
                            break;
                        case "Encoding":
                            string enc = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            encoding = (FileEncoding)Enum.Parse(typeof(FileEncoding), enc);
                            break;
                        case "Order":
                            order = Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
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
                id = "frx" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(regex))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Regex"));
            }
            if (string.IsNullOrEmpty(fileId) == string.IsNullOrEmpty(filepath))
            {
                // Either under File or specify FilePath, not both
                Core.OnMessage(WixErrors.IllegalAttributeWhenNested(sourceLineNumbers, node.LocalName, "FilePath", "Product"));
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
                else if (((XmlNodeType.CDATA == child.NodeType) || (XmlNodeType.Text == child.NodeType)) && string.IsNullOrEmpty(condition))
                {
                    Core.OnMessage(WixWarnings.DeprecatedElement(sourceLineNumbers, "text", $"Condition attribute in {node.LocalName}"));
                    condition = child.Value.Trim();
                }
            }

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "FileRegex_Immediate");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_FileRegex");
                row[0] = id;
                row[1] = component;
                row[2] = fileId;
                row[3] = filepath;
                row[4] = regex;
                row[5] = replacement ?? "";
                row[6] = ignoreCase ? 1 : 0;
                row[7] = (int)encoding;
                row[8] = condition;
                row[9] = order;
            }
        }

        [Flags]
        private enum DeletePathFlags
        {
            IgnoreMissing = 1,
            IgnoreErrors = 2 * IgnoreMissing,
            OnlyIfEmpty = 2 * IgnoreErrors,
            AllowReboot = 2 * OnlyIfEmpty
        }

        private void ParseDeletePath(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string filepath = null;
            string condition = null;
            DeletePathFlags flags = DeletePathFlags.AllowReboot | DeletePathFlags.IgnoreErrors | DeletePathFlags.IgnoreMissing;
            int order = 1000000000 + GetLineNumber(sourceLineNumbers);

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == schema.TargetNamespace)
                {
                    switch (attrib.LocalName)
                    {
                        case "Id":
                            id = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Path":
                            filepath = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreMissing":
                        case "IgnoreErrors":
                            Core.OnMessage(WixWarnings.DeprecatedAttribute(sourceLineNumbers, node.LocalName, attrib.LocalName));
                            break;
                        case "OnlyIfEmpty":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.OnlyIfEmpty;
                            }
                            break;
                        case "AllowReboot":
                            if (Core.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.No)
                            {
                                flags &= ~DeletePathFlags.AllowReboot;
                            }
                            break;
                        case "Condition":
                            condition = Core.GetAttributeValue(sourceLineNumbers, attrib);
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
                id = "dlt" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(filepath))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "Path"));
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

            if (flags.HasFlag(DeletePathFlags.AllowReboot))
            {
                Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "PSW_CheckRebootRequired");
            }
            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "DeletePath");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_DeletePath");
                row[0] = id;
                row[1] = filepath;
                row[2] = (int)flags;
                row[3] = condition;
                row[4] = order;
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
            ErrorHandling promptOnError = ErrorHandling.fail;

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
                        case "ErrorHandling":
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
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
                id = "zip" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(dstZipFile))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "TargetZipFile"));
            }
            if (string.IsNullOrEmpty(srcDir))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "SourceFolder"));
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

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ZipFileSched");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_ZipFile");
                row[0] = id;
                row[1] = dstZipFile;
                row[2] = srcDir;
                row[3] = filePattern;
                row[4] = recursive ? 1 : 0;
                row[5] = condition;
                row[6] = (int)promptOnError;
            }
        }

        [Flags]
        enum UnzipFlags
        {
            // Overwrite mode
            Never = 0,
            Always = 0x1,
            Unmodified = 0x2,
            OverwriteMask = 0x3,

            // Delete ZIP after extract
            Delete = 0x10,

            // 
            CreateRoot = 0x20,

            OnRollback = 0x40,
        };

        private void ParseUnzip(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string zipFile = null;
            string dstDir = null;
            string condition = null;
            UnzipFlags flags = UnzipFlags.Unmodified | UnzipFlags.CreateRoot;
            ErrorHandling promptOnError = ErrorHandling.fail;

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
                        case "CreateRootFolder":
                            {
                                YesNoType aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.No)
                                {
                                    flags &= ~UnzipFlags.CreateRoot;
                                }
                            }
                            break;
                        case "DeleteZip":
                            {
                                YesNoType aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.Yes)
                                {
                                    flags |= UnzipFlags.Delete;
                                }
                            }
                            break;
                        case "OnRollback":
                            {
                                YesNoType aye = Core.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.Yes)
                                {
                                    flags |= UnzipFlags.OnRollback;
                                }
                            }
                            break;
                        case "Overwrite":
                            {
                                YesNoDefaultType aye = Core.GetAttributeYesNoDefaultValue(sourceLineNumbers, attrib);
                                flags = ((aye == YesNoDefaultType.Yes) ? ((flags & ~UnzipFlags.OverwriteMask) | UnzipFlags.Always)
                                    : (aye == YesNoDefaultType.No) ? (flags & ~UnzipFlags.OverwriteMask)
                                    : ((flags & ~UnzipFlags.OverwriteMask) | UnzipFlags.Unmodified));
                            }
                            break;
                        case "OverwriteMode":
                            string b = Core.GetAttributeValue(sourceLineNumbers, attrib);
                            try
                            {
                                UnzipFlags f = (UnzipFlags)Enum.Parse(typeof(UnzipFlags), b);
                                flags = ((flags & ~UnzipFlags.OverwriteMask) | f);
                            }
                            catch
                            {
                                Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            }
                            break;
                        case nameof(ErrorHandling):
                            {
                                string a = Core.GetAttributeValue(sourceLineNumbers, attrib);
                                try
                                {
                                    promptOnError = (ErrorHandling)Enum.Parse(typeof(ErrorHandling), a);
                                }
                                catch
                                {
                                    Core.UnexpectedAttribute(sourceLineNumbers, attrib);
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
                id = "uzp" + Guid.NewGuid().ToString("N");
            }
            if (string.IsNullOrEmpty(zipFile))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "ZipFile"));
            }
            if (string.IsNullOrEmpty(dstDir))
            {
                Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.LocalName, "TargetFolder"));
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

            Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "UnzipSched");

            if (!Core.EncounteredError)
            {
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Unzip");
                row[0] = id;
                row[1] = zipFile;
                row[2] = dstDir;
                row[3] = (int)flags;
                row[4] = condition;
                row[5] = (int)promptOnError;
            }
        }
    }
}
