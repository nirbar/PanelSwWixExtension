using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml;
using System.Xml.Linq;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    /// <summary>
    /// The compiler for the Windows Installer XML Toolset PanelSwWixExtension Extension.
    /// </summary>
    public sealed class PanelSwWixCompiler : BaseCompilerExtension
    {
        public override XNamespace Namespace => XNamespace.Get("http://schemas.panel-sw.co.il/wix/WixExtension");

        /// <summary>
        /// Processes an element for the Compiler.
        /// </summary>
        /// <param name="sourceLineNumbers">Source line number for the parent element.</param>
        /// <param name="parentElement">Parent element of element to process.</param>
        /// <param name="element">Element to process.</param>
        /// <param name="contextValues">Extra information about the context in which this element is being parsed.</param>
        public override void ParseElement(Intermediate intermediate, IntermediateSection section, XElement parentElement, XElement element, IDictionary<string, string> context)
        {
            switch (parentElement.Name.LocalName)
            {
                case "Bundle":
                    switch (element.Name.LocalName)
                    {
                        case "ContainerTemplate":
                            ParseContainerTemplateElement(section, element);
                            break;

                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Fragment":
                case "Module":
                case "Package":
                    switch (element.Name.LocalName)
                    {
                        case "CustomUninstallKey":
                            ParseCustomUninstallKeyElement(section, element);
                            break;

                        case "Payload":
                            ParsePayload(section, element, null, null);
                            break;

                        case "FileRegex":
                            ParseFileRegex(section, element, null, null);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(section, element, parentElement);
                            break;

                        case "RemoveRegistryValue":
                            ParseRemoveRegistryValue(section, element);
                            break;

                        case "Telemetry":
                            ParseTelemetry(section, element);
                            break;

                        case "ShellExecute":
                            ParseShellExecute(section, element);
                            break;

                        case "MsiSqlQuery":
                            ParseMsiSqlQuery(section, element, parentElement);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(section, element);
                            break;

                        case "RestartLocalResources":
                            ParseRestartLocalResources(section, element);
                            break;

                        case "DeletePath":
                            ParseDeletePath(section, element);
                            break;

                        case "ZipFile":
                            ParseZipFile(section, element);
                            break;

                        case "Unzip":
                            ParseUnzip(section, element);
                            break;

                        case "SetPropertyFromPipe":
                            ParseSetPropertyFromPipe(section, element);
                            break;

                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "StandardDirectory":
                case "DirectoryRef":
                case "Directory":
                    {
                        switch (element.Name.LocalName)
                        {
                            case "DiskSpace":
                                ParseDiskSpaceElement(section, parentElement, element);
                                break;

                            default:
                                ParseHelper.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "Property":
                    switch (element.Name.LocalName)
                    {
                        case "AccountSidSearch":
                            ParseAccountSidSearchElement(section, element);
                            break;

                        case "PathSearch":
                            ParsePathSearchElement(section, element);
                            break;

                        case "VersionCompare":
                            ParseVersionCompareElement(section, element);
                            break;

                        case "XmlSearch":
                            ParseXmlSearchElement(section, element);
                            break;

                        case "WmiSearch":
                            ParseWmiSearchElement(section, element);
                            break;

                        case "SqlSearch":
                            ParseSqlSearchElement(section, element);
                            break;

                        case "Evaluate":
                            ParseEvaluateElement(section, element);
                            break;

                        case "CertificateHashSearch":
                            ParseCertificateHashSearchElement(section, element);
                            break;

                        case "JsonJpathSearch":
                            ParseJsonJpathSearchElement(section, element);
                            break;

                        case "MsiSqlQuery":
                            ParseMsiSqlQuery(section, element, parentElement);
                            break;

                        case "ReadIniValues":
                            ParseReadIniValuesElement(section, element, parentElement);
                            break;

                        case "RegularExpression":
                            ParseRegularExpression(section, element);
                            break;

                        case "ToLowerCase":
                            ParseToLowerCase(section, element);
                            break;

                        case "Md5Hash":
                            ParseMd5Hash(section, element);
                            break;

                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Component":
                    {
                        string componentId = context["ComponentId"];
                        string directoryId = context["DirectoryId"];
                        bool isWin64 = bool.Parse(context["Win64"]);

                        switch (element.Name.LocalName)
                        {
                            case "JsonJPath":
                                ParseJsonJPathElement(section, element, componentId, null);
                                break;

                            case "TaskScheduler":
                                ParseTaskSchedulerElement(section, element, componentId);
                                break;

                            case "ExecOn":
                            case "ExecOnComponent":
                                ParseExecOnComponentElement(section, element, componentId);
                                break;

                            case "Dism":
                                ParseDismElement(section, element, componentId);
                                break;

                            case "ServiceConfig":
                                ParseServiceConfigElement(section, element, componentId);
                                break;

                            case "BackupAndRestore":
                                ParseBackupAndRestoreElement(section, element, componentId);
                                break;

                            case "CreateSelfSignCertificate":
                                ParseCreateSelfSignCertificateElement(section, element, componentId);
                                break;

                            case "SqlScript":
                                ParseSqlScriptElement(section, element, componentId);
                                break;

                            case "WebsiteConfig":
                                ParseWebsiteConfigElement(section, element, componentId);
                                break;

                            case "XslTransform":
                                ParseXslTransform(section, element, componentId, null);
                                break;

                            default:
                                ParseHelper.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "File":
                    {
                        string fileId = context["FileId"];
                        string componentId = context["ComponentId"];
                        bool isWin64 = bool.Parse(context["Win64"]);
                        string directoryId = context["DirectoryId"];

                        switch (element.Name.LocalName)
                        {
                            case "JsonJPath":
                                ParseJsonJPathElement(section, element, componentId, fileId);
                                break;

                            case "InstallUtil":
                                ParseInstallUtilElement(section, element, fileId);
                                break;

                            case "TopShelf":
                                ParseTopShelfElement(section, element, fileId);
                                break;

                            case "ForceVersion":
                                ParseForceVersionElement(section, element, fileId);
                                break;

                            case "FileRegex":
                                ParseFileRegex(section, element, componentId, fileId);
                                break;

                            case "XslTransform":
                                ParseXslTransform(section, element, componentId, fileId);
                                break;

                            case "SplitFile":
                                ParseSplitFileElement(section, parentElement, element, componentId, fileId);
                                break;

                            default:
                                ParseHelper.UnexpectedElement(parentElement, element);
                                break;
                        }
                        break;
                    }

                case "PatchFamily":
                    switch (element.Name.LocalName)
                    {
                        case "CustomPatchRef":
                            ParseCustomPatchRefElement(section, element);
                            break;
                        default:
                            ParseHelper.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                default:
                    ParseHelper.UnexpectedElement(parentElement, element);
                    break;
            }
        }

        private void ParseContainerTemplateElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string cabinetTemplate = "bundle-attached-{0}.cab";
            ContainerType defaultType = ContainerType.Attached;
            int maximumUncompressedContainerSize = Int32.MaxValue; // 2GB
            long maximumUncompressedExeSize = -1; // 4GB

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "CabinetTemplate":
                            cabinetTemplate = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "DefaultType":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out defaultType);
                            break;
                        case "MaximumUncompressedContainerSize":
                            maximumUncompressedContainerSize = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 1, Int32.MaxValue);
                            break;
                        case "MaximumUncompressedExeSize":
                            maximumUncompressedExeSize = ParseHelper.GetAttributeLongValue(sourceLineNumbers, attrib, 1, UInt32.MaxValue);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (!cabinetTemplate.Contains("{0}"))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeValue(sourceLineNumbers, element.Name.LocalName, "CabinetTemplate", cabinetTemplate, "Must contain format string {0}"));
            }
            if ((defaultType == ContainerType.Detached) && (maximumUncompressedExeSize > 0))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "DefaultType", defaultType.ToString(), "MaximumUncompressedExeSize"));
            }
            if (maximumUncompressedExeSize < 0)
            {
                maximumUncompressedExeSize = UInt32.MaxValue; //4GB
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                PSW_ContainerTemplate symbol = section.AddSymbol(new PSW_ContainerTemplate(sourceLineNumbers));
                symbol.CabinetTemplate = cabinetTemplate;
                symbol.DefaultType = defaultType;
                symbol.MaximumUncompressedContainerSize = maximumUncompressedContainerSize;
                symbol.MaximumUncompressedExeSize = maximumUncompressedExeSize;
            }
        }

        private void ParsePayload(IntermediateSection section, XElement element, object p1, object p2)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string source = null;
            string name = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Source":
                            source = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Name":
                            name = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(source))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Source"));
                return;
            }
            if (string.IsNullOrEmpty(name))
            {
                name = Path.GetFileName(source);
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ExtractPayload");

            string binaryKey = $"pld{Guid.NewGuid().ToString("N")}";
            BinarySymbol binaryRow = section.AddSymbol(new BinarySymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, binaryKey)));
            binaryRow.Data = new IntermediateFieldPathValue() { Path = source };

            PSW_Payload pldRow = section.AddSymbol(new PSW_Payload(sourceLineNumbers, binaryKey));
            pldRow.Name = name;
        }

        private void ParseSplitFileElement(IntermediateSection section, XElement fileElement, XElement element, string componentId, string fileId)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            int splitSize = Int32.MaxValue; //2GB

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Size":
                            splitSize = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 1, Int32.MaxValue);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            XAttribute sourceAttrib = fileElement.Attribute("Source");
            if (sourceAttrib == null)
            {
                sourceAttrib = fileElement.Attribute("src");
            }
            string sourcePath = ParseHelper.GetAttributeValue(sourceLineNumbers, sourceAttrib);
            if (!File.Exists(sourcePath))
            {
                Messaging.Write(ErrorMessages.FileNotFound(sourceLineNumbers, sourcePath));
                return;
            }
            FileInfo fileInfo = new FileInfo(sourcePath);
            if (fileInfo.Length <= splitSize)
            {
                return;
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ConcatFiles");

            string tmpPath = Path.GetTempPath();
            int splitCnt = (int)Math.Ceiling(1m * fileInfo.Length / splitSize);
            for (int i = 1; i < splitCnt; ++i)
            {
                XElement splitFileElement = new XElement(fileElement);
                string splId = "spl" + Guid.NewGuid().ToString("N");
                string splFile = Path.Combine(tmpPath, splId);
                File.Create(splFile).Dispose();

                splitFileElement.SetAttributeValue("KeyPath", "no");
                splitFileElement.SetAttributeValue("CompanionFile", fileId);
                splitFileElement.SetAttributeValue("Name", splId);
                splitFileElement.SetAttributeValue("Id", splId);
                splitFileElement.SetAttributeValue("Source", splFile);

                fileElement.Parent.Add(splitFileElement);

                if (CheckNoCData(element) && !Messaging.EncounteredError)
                {
                    PSW_ConcatFiles row = section.AddSymbol(new PSW_ConcatFiles(sourceLineNumbers));
                    row.Component_ = componentId;
                    row.RootFile_ = fileId;
                    row.MyFile_ = splId;
                    row.Order = i;
                    row.Size = splitSize;
                }
            }
        }

        private void ParseCustomPatchRefElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string table = null;
            string key = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Table":
                            table = ParseHelper.GetAttributeIdentifierValue(sourceLineNumbers, attrib);
                            break;
                        case "Key":
                            key = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(table))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Table"));
            }
            if (string.IsNullOrEmpty(key))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Key"));
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
                    customActions.Add("ExecOnComponent_Imp_BeforeStop_rollback");
                    customActions.Add("ExecOnComponent_Imp_BeforeStop_deferred");
                    customActions.Add("ExecOnComponent_Imp_AfterStop_rollback");
                    customActions.Add("ExecOnComponent_Imp_AfterStop_deferred");
                    customActions.Add("ExecOnComponent_Imp_BeforeStart_rollback");
                    customActions.Add("ExecOnComponent_Imp_BeforeStart_deferred");
                    customActions.Add("ExecOnComponent_Imp_AfterStart_rollback");
                    customActions.Add("ExecOnComponent_Imp_AfterStart_deferred");
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

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                WixPatchRefSymbol patchReferenceRow;
                foreach (string ca in customActions)
                {
                    patchReferenceRow = section.AddSymbol(new WixPatchRefSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, "CustomAction")));
                    patchReferenceRow.Table = "CustomAction";
                    patchReferenceRow.PrimaryKeys = ca;
                }

                patchReferenceRow = section.AddSymbol(new WixPatchRefSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, "Binary")));
                patchReferenceRow.Table = "Binary";
                patchReferenceRow.PrimaryKeys = "PanelSwCustomActions.dll";

                patchReferenceRow = section.AddSymbol(new WixPatchRefSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, table)));
                patchReferenceRow.Table = table;
                patchReferenceRow.PrimaryKeys = key;
            }
        }

        public override void ParseAttribute(Intermediate intermediate, IntermediateSection section, XElement parentElement, XAttribute attribute, IDictionary<string, string> context)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(parentElement);

            switch (parentElement.Name.LocalName)
            {
                case "CustomAction":
                    switch (attribute.Name.LocalName)
                    {
                        case "CustomActionData":
                            ParseCustomActionDataAttribute(section, parentElement, attribute);
                            break;

                        case "ActionStartText":
                            ParseActionStartTextAttribute(section, parentElement, attribute);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(parentElement, attribute);
                            break;
                    }
                    break;

                default:
                    ParseHelper.UnexpectedAttribute(parentElement, attribute);
                    break;
            }
        }

        private void ParseActionStartTextAttribute(IntermediateSection section, XElement parentElement, XAttribute attribute)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(parentElement);
            string actionStartText = ParseHelper.GetAttributeValue(sourceLineNumbers, attribute);
            if (string.IsNullOrEmpty(actionStartText))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, attribute.Name.LocalName));
                return;
            }

            XAttribute idAttrib = parentElement.Attribute("Id");
            if (idAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
                return;
            }
            string caId = ParseHelper.GetAttributeIdentifierValue(sourceLineNumbers, idAttrib);
            if (string.IsNullOrEmpty(caId))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
                return;
            }

            ActionTextSymbol row = section.AddSymbol(new ActionTextSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, caId)));
            row.Action = caId;
            row.Description = actionStartText;
            row.Template = "";
        }

        private void ParseCustomActionDataAttribute(IntermediateSection section, XElement parentElement, XAttribute attribute)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(parentElement);
            string cad = ParseHelper.GetAttributeValue(sourceLineNumbers, attribute);
            if (string.IsNullOrEmpty(cad))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, attribute.Name.LocalName));
            }

            XAttribute executeAttrib = parentElement.Attribute("Execute");
            if (executeAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Execute"));
                return;
            }
            string execute = ParseHelper.GetAttributeValue(sourceLineNumbers, executeAttrib);
            if (string.IsNullOrEmpty(execute) || !(execute.Equals("commit") || execute.Equals("deferred") || execute.Equals("rollback")))
            {
                // CustomActionData is only relevant for deferred actions
                Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Execute", execute, attribute.Name.LocalName));
                return;
            }

            XAttribute dllEntryAttrib = parentElement.Attribute("DllEntry");
            if (dllEntryAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "DllEntry"));
            }
            string dllEntry = ParseHelper.GetAttributeValue(sourceLineNumbers, dllEntryAttrib);
            if (string.IsNullOrWhiteSpace(dllEntry))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "DllEntry"));
                return;
            }

            XAttribute idAttrib = parentElement.Attribute("Id");
            if (idAttrib == null)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
            }
            string caId = ParseHelper.GetAttributeIdentifierValue(sourceLineNumbers, idAttrib);
            if (string.IsNullOrEmpty(caId))
            {
                Messaging.Write(ErrorMessages.IllegalEmptyAttributeValue(sourceLineNumbers, parentElement.Name.LocalName, "Id"));
            }

            if (Messaging.EncounteredError)
            {
                return;
            }

            CustomActionSymbol row = section.AddSymbol(new CustomActionSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, $"Set{caId}")));
            row.ExecutionType = CustomActionExecutionType.Immediate;
            row.Source = caId;
            row.SourceType = CustomActionSourceType.Property;
            row.Target = cad;
            row.TargetType = CustomActionTargetType.TextData;

            WixActionSymbol sequenceRow = section.AddSymbol(new WixActionSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, $"InstallExecuteSequence/Set{caId}")));
            sequenceRow.Action = $"Set{caId}";
            sequenceRow.SequenceTable = SequenceTable.InstallExecuteSequence;
            sequenceRow.Condition = null;
            sequenceRow.Before = caId;
            sequenceRow.After = null;
            sequenceRow.Overridable = false;

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", caId);

            // Set action text to the SetProperty element as well if it is set for the CA
            XAttribute caTextAttrib = parentElement.Attribute(XName.Get("ActionStartText", attribute.Name.NamespaceName));
            if (caTextAttrib != null)
            {
                string actionText = ParseHelper.GetAttributeValue(sourceLineNumbers, caTextAttrib);
                if (!string.IsNullOrWhiteSpace(actionText))
                {
                    string id = $"Set{caId}";
                    ActionTextSymbol actionText1 = section.AddSymbol(new ActionTextSymbol(sourceLineNumbers, new Identifier(AccessModifier.Global, id)));
                    actionText1.Action = id;
                    actionText1.Description = actionText;
                    actionText1.Template = "";

                    ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", id);
                }
            }
        }

        private void ParseWebsiteConfigElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string id = "web" + Guid.NewGuid().ToString("N");
            string website = null;
            bool stop = false;
            bool start = false;
            YesNoDefaultType autoStart = YesNoDefaultType.Default;
            ErrorHandling promptOnError = ErrorHandling.fail;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Website":
                            website = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Stop":
                            stop = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;

                        case "Start":
                            start = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;

                        case "AutoStart":
                            autoStart = ParseHelper.GetAttributeYesNoDefaultValue(sourceLineNumbers, attrib);
                            break;

                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out promptOnError);
                            break;

                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(website))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Website"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_WebsiteConfigSched");
                PSW_WebsiteConfig row = section.AddSymbol(new PSW_WebsiteConfig(sourceLineNumbers, id));
                row.Component_ = component;
                row.Website = website;
                row.Stop = stop ? 1 : 0;
                row.Start = start ? 1 : 0;
                row.AutoStart = (autoStart == YesNoDefaultType.Yes) ? 1 : (autoStart == YesNoDefaultType.No) ? 0 : -1;
                row.ErrorHandling = (int)promptOnError;
                row.Order = order;
            }
        }

        private void ParseMd5Hash(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string property = null;
            string plain = null;

            if (element.Parent.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(element.Parent, element);
            }
            property = element.Parent.Attribute("Id").Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Plain":
                            plain = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Parent.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(plain))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Plain"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "Md5Hash");

                PSW_Md5Hash row = section.AddSymbol(new PSW_Md5Hash(sourceLineNumbers));
                row.Property_ = property;
                row.Plain = plain;
            }
        }

        private void ParseToLowerCase(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string property = null;

            if (element.Parent.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(element.Parent, element);
            }
            property = element.Parent.Attribute("Id").Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Parent.Name.LocalName, "Id"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_ToLowerCase");

                section.AddSymbol(new PSW_ToLowerCase(sourceLineNumbers, property));
            }
        }

        private void ParseJsonJpathSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string property = null;
            string expression = null;
            string file = null;

            if (element.Parent.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(element.Parent, element);
            }
            property = element.Parent.Attribute("Id").Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "JPath":
                            expression = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePath":
                            file = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Parent.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "JPath"));
            }
            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "FilePath"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "JsonJpathSearch");

                PSW_JsonJpathSearch row = section.AddSymbol(new PSW_JsonJpathSearch(sourceLineNumbers));
                row.Property_ = property;
                row.JPath = expression;
                row.FilePath = file;
            }
        }

        private enum JsonFormatting
        {
            Raw,
            String,
            Boolean
        }

        private void ParseJsonJPathElement(IntermediateSection section, XElement element, string component_, string file_)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string jpath = null;
            string value = null;
            string filePath = null;
            JsonFormatting jsonFormatting = JsonFormatting.Raw;
            ErrorHandling promptOnError = ErrorHandling.fail;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "JPath":
                            jpath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FilePath":
                            filePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Value":
                            value = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Formatting":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out jsonFormatting);
                            break;

                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out promptOnError);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file_) == string.IsNullOrEmpty(filePath))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWhenNested(sourceLineNumbers, element.Name.LocalName, "FilePath", "File"));
            }
            if (string.IsNullOrEmpty(jpath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "JPath"));
            }
            if (string.IsNullOrEmpty(value))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Value"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "JsonJpathSched");

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                PSW_JsonJPath row = section.AddSymbol(new PSW_JsonJPath(sourceLineNumbers));
                row.Component_ = component_;
                row.FilePath = filePath;
                row.File_ = file_;
                row.JPath = jpath;
                row.Value = value;
                row.Formatting = (int)jsonFormatting;
                row.ErrorHandling = (int)promptOnError;
            }
        }

        private void ParseDiskSpaceElement(IntermediateSection section, XElement parentElement, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            Identifier directory;
            string directoryId;

            XAttribute idAttrib = parentElement.Attribute("Id");
            if (idAttrib == null)
            {
                Messaging.Write(ErrorMessages.ParentElementAttributeRequired(sourceLineNumbers, parentElement.Name.LocalName, "Id", element.Name.LocalName));
                return;
            }
            directory = ParseHelper.GetAttributeIdentifier(sourceLineNumbers, idAttrib);
            if (directory == null)
            {
                Messaging.Write(ErrorMessages.ParentElementAttributeRequired(sourceLineNumbers, parentElement.Name.LocalName, "Id", element.Name.LocalName));
                return;
            }
            directoryId = WindowsInstallerStandard.GetPlatformSpecificDirectoryId(directory.Id, Context.Platform);

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "DiskSpace");
                section.AddSymbol(new PSW_DiskSpace(sourceLineNumbers, directoryId));
            }
        }

        private void ParseSetPropertyFromPipe(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string pipe = null;
            int timeout = 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "PipeName":
                            pipe = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Timeout":
                            timeout = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, int.MaxValue);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(pipe))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "PipeName"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "SetPropertyFromPipe");
                PSW_SetPropertyFromPipe row = section.AddSymbol(new PSW_SetPropertyFromPipe(sourceLineNumbers));
                row.PipeName = pipe;
                row.Timeout = timeout;
            }
        }

        private void ParseCreateSelfSignCertificateElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            Identifier id = null;
            string password = null;
            string x500 = null;
            string subjectAltName = null;
            ushort expiry = 0;
            bool deleteOnCommit = true;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Id":
                            id = ParseHelper.GetAttributeIdentifier(sourceLineNumbers, attrib);
                            break;
                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Expiry":
                            expiry = (ushort)ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, ushort.MaxValue);
                            break;
                        case "X500":
                            x500 = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "DeleteOnCommit":
                            deleteOnCommit = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;
                        case "SubjectAltName":
                            subjectAltName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(id?.Id))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(x500))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "X500"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CreateSelfSignCertificate");

                PSW_SelfSignCertificate row = section.AddSymbol(new PSW_SelfSignCertificate(sourceLineNumbers, id.Id));
                row.Component_ = component;
                row.X500 = x500;
                row.SubjectAltNames = subjectAltName;
                row.Expiry = expiry;
                row.Password = password;
                row.DeleteOnCommit = deleteOnCommit ? 1 : 0;
            }
        }

        private enum BackupAndRestore_deferred_Schedule
        {
            BackupAndRestore_deferred_Before_InstallFiles,
            BackupAndRestore_deferred_After_DuplicateFiles,
            BackupAndRestore_deferred_After_RemoveExistingProducts
        }

        private void ParseBackupAndRestoreElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string filepath = null;
            BackupAndRestore_deferred_Schedule restoreSchedule = BackupAndRestore_deferred_Schedule.BackupAndRestore_deferred_Before_InstallFiles;
            DeletePathFlags flags = 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Path":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreMissing":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.IgnoreMissing;
                            }
                            break;
                        case "IgnoreErrors":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.IgnoreErrors;
                            }
                            break;
                        case "RestoreScheduling":
                            {
                                string val = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
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
                                        Messaging.Write(ErrorMessages.ValueNotSupported(sourceLineNumbers, element.Name.LocalName, attrib.Name.LocalName, val));
                                        break;
                                }
                            }
                            break;


                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(filepath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Path"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "BackupAndRestore");
            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "Property", restoreSchedule.ToString());

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                PSW_BackupAndRestore row = section.AddSymbol(new PSW_BackupAndRestore(sourceLineNumbers));
                row.Component_ = component;
                row.Path = filepath;
                row.Flags = (ushort)flags;
            }
        }

        enum InstallUtil_Bitness
        {
            asComponent = 0,
            x86 = 1,
            x64 = 2
        }

        private void ParseInstallUtilElement(IntermediateSection section, XElement element, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            InstallUtil_Bitness bitness = InstallUtil_Bitness.asComponent;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Bitness":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out bitness);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "File", "Id"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                // Ensure sub-table exists for queries to succeed even if no sub-entries exist.
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_InstallUtil_Arg");

                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_InstallUtilSched");
                PSW_InstallUtil row = section.AddSymbol(new PSW_InstallUtil(sourceLineNumbers, file));
                row.Bitness = (ushort)bitness;
            }

            // Iterate child 'Argument' elements
            foreach (XElement child in element.Descendants())
            {
                if (child.Name.Namespace.Equals(Namespace) && !child.Name.LocalName.Equals("Argument"))
                {
                    Messaging.Write(ErrorMessages.UnsupportedExtensionElement(sourceLineNumbers, element.Name.LocalName, child.Name.LocalName));
                    continue;
                }

                string value = null;
                foreach (XAttribute attrib in child.Attributes())
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Value":
                            value = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(child, attrib);
                            break;
                    }
                }

                if (string.IsNullOrEmpty(value))
                {
                    Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, child.Name.LocalName, "Value"));
                    continue;
                }

                if (CheckNoCData(child) && !Messaging.EncounteredError)
                {
                    PSW_InstallUtil_Arg row = section.AddSymbol(new PSW_InstallUtil_Arg(sourceLineNumbers, file));
                    row.Value = value;
                }
            }
        }

        private void ParseForceVersionElement(IntermediateSection section, XElement element, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string version = "65535.65535.65535.65535";

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Version":
                            version = ParseHelper.GetAttributeVersionValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "File", "Id"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ForceVersion");

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                PSW_ForceVersion row = section.AddSymbol(new PSW_ForceVersion(sourceLineNumbers, file));
                row.Version = version;
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
            prompt = 2
        }

        private void ParseTopShelfElement(IntermediateSection section, XElement element, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            TopShelf_Account account = TopShelf_Account.none;
            TopShelf_Start start = TopShelf_Start.none;
            string serviceName = null;
            string displayName = null;
            string description = null;
            string instance = null;
            string userName = null;
            string password = null;
            ErrorHandling promptOnError = ErrorHandling.fail;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Account":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out account);
                            break;

                        case "Start":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out start);
                            break;

                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out promptOnError);
                            break;

                        case "ServiceName":
                            serviceName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "DisplayName":
                            displayName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Description":
                            description = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Instance":
                            instance = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "UserName":
                            userName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "File", "Id"));
            }
            if (string.IsNullOrEmpty(userName) != (account != TopShelf_Account.custom))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, "TopShelf", "Account", "custom", "UserName"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "TopShelf");

                PSW_TopShelf row = section.AddSymbol(new PSW_TopShelf(sourceLineNumbers, file));
                row.ServiceName = serviceName;
                row.DisplayName = displayName;
                row.Description = description;
                row.Instance = instance;
                row.Account = (int)account;
                row.UserName = userName;
                row.Password = password;
                row.HowToStart = (int)start;
                row.ErrorHandling = (int)promptOnError;
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

        private void ParseSqlScriptElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
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
            ErrorHandling errorHandling = ErrorHandling.fail;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            SqlExecOn sqlExecOn = SqlExecOn.None;
            YesNoType aye;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "BinaryKey":
                            binary = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "ConnectionString":
                            connectionString = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Driver":
                            driver = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Server":
                            server = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Instance":
                            instance = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Port":
                            port = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Encrypt":
                            encrypted = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Database":
                            database = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Username":
                            username = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out errorHandling);
                            break;

                        case "OnInstall":
                            aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                sqlExecOn |= SqlExecOn.Install;
                            }
                            break;

                        case "OnInstallRollback":
                            aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                sqlExecOn |= SqlExecOn.InstallRollback;
                            }
                            break;

                        case "OnReinstall":
                            aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                sqlExecOn |= SqlExecOn.Reinstall;
                            }
                            break;

                        case "OnReinstallRollback":
                            aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                sqlExecOn |= SqlExecOn.ReinstallRollback;
                            }
                            break;

                        case "OnUninstall":
                            aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                sqlExecOn |= SqlExecOn.Uninstall;
                            }
                            break;

                        case "OnUninstallRollback":
                            aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            if (aye == YesNoType.Yes)
                            {
                                sqlExecOn |= SqlExecOn.UninstallRollback;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (!string.IsNullOrEmpty(connectionString) &&
                (!string.IsNullOrEmpty(server) || !string.IsNullOrEmpty(instance) || !string.IsNullOrEmpty(database) || !string.IsNullOrEmpty(port) || !string.IsNullOrEmpty(encrypted) || !string.IsNullOrEmpty(password) || !string.IsNullOrEmpty(username)))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "ConnectionString", "any other"));
            }
            if (string.IsNullOrEmpty(server) && string.IsNullOrEmpty(connectionString))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "Server", "ConnectionString"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(binary))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "BinaryKey"));
            }
            if (sqlExecOn == SqlExecOn.None)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "OnXXX"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_SqlScript");

            PSW_SqlScript row = null;
            if (!Messaging.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_SqlScript_Replacements");
                row = section.AddSymbol(new PSW_SqlScript(sourceLineNumbers));
                row.Component_ = component;
                row.Binary_ = binary;
                row.Server = server;
                row.Instance = instance;
                row.Port = port;
                row.Encrypted = encrypted;
                row.Database = database;
                row.Username = username;
                row.Password = password;
                row.On = (int)sqlExecOn;
                row.ErrorHandling = (int)errorHandling;
                row.Order = order;
                row.ConnectionString = connectionString;
                row.Driver = driver;
            }

            // ExitCode mapping
            foreach (XElement child in element.Descendants())
            {
                SourceLineNumber repLines = ParseHelper.GetSourceLineNumbers(child);
                if (child.Name.Namespace.Equals(Namespace))
                {
                    switch (child.Name.LocalName)
                    {
                        case "Replace":
                            {
                                string from = null;
                                string to = "";
                                int repOrder = 1000000000 + repLines.LineNumber ?? 0;
                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (IsMyAttribute(child, a))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Text":
                                                from = ParseHelper.GetAttributeValue(repLines, a);
                                                break;

                                            case "Replacement":
                                                to = ParseHelper.GetAttributeValue(repLines, a, EmptyRule.CanBeEmpty);
                                                break;

                                            case "Order":
                                                repOrder = ParseHelper.GetAttributeIntegerValue(repLines, a, -1000000000, 1000000000);
                                                if (repOrder < 0)
                                                {
                                                    repOrder += int.MaxValue;
                                                }
                                                break;
                                        }
                                    }
                                }

                                if (CheckNoCData(child) && !Messaging.EncounteredError)
                                {
                                    PSW_SqlScript_Replacements row1 = section.AddSymbol(new PSW_SqlScript_Replacements(repLines, row?.Id));
                                    row1.Text = from;
                                    row1.Replacement = to;
                                    row1.Order = repOrder;
                                }
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedElement(element, child);
                            break;
                    }
                }
            }
        }

        enum InstallUninstallType
        {
            install,
            uninstall,
            both
        }

        private void ParseXslTransform(IntermediateSection section, XElement element, string component, string file)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string filePath = null;
            string binary = null;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            InstallUninstallType on = InstallUninstallType.install;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "BinaryKey":
                            binary = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FilePath":
                            filePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "On":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out on);
                            break;

                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file) == string.IsNullOrEmpty(filePath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributeInElementOrParent(sourceLineNumbers, element.Name.LocalName, "FilePath", "File"));
            }
            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(binary))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "BinaryKey"));
            }

            PSW_XslTransform row = null;
            if (!Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_XslTransform");
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_XslTransform_Replacements");
                row = section.AddSymbol(new PSW_XslTransform(sourceLineNumbers));
                row.File_ = file;
                row.Component_ = component;
                row.FilePath = filePath;
                row.XslBinary_ = binary;
                row.Order = order;
                row.On = (int)on;
            }

            // Text replacements in XSL
            foreach (XElement child in element.Descendants())
            {
                SourceLineNumber repLines = ParseHelper.GetSourceLineNumbers(child);
                if (child.Name.Namespace.Equals(Namespace))
                {
                    switch (child.Name.LocalName)
                    {
                        case "Replace":
                            {
                                string from = null;
                                string to = "";
                                int repOrder = 1000000000 + repLines.LineNumber ?? 0;
                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (IsMyAttribute(child, a))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Text":
                                                from = ParseHelper.GetAttributeValue(repLines, a);
                                                break;

                                            case "Replacement":
                                                to = ParseHelper.GetAttributeValue(repLines, a, EmptyRule.CanBeEmpty);
                                                break;

                                            case "Order":
                                                repOrder = ParseHelper.GetAttributeIntegerValue(repLines, a, -1000000000, 1000000000);
                                                if (repOrder < 0)
                                                {
                                                    repOrder += int.MaxValue;
                                                }
                                                break;
                                        }
                                    }
                                }

                                if (CheckNoCData(child) && !Messaging.EncounteredError)
                                {
                                    PSW_XslTransform_Replacements row1 = section.AddSymbol(new PSW_XslTransform_Replacements(repLines, row.Id));
                                    row1.Text = from;
                                    row1.Replacement = to;
                                    row1.Order = repOrder;
                                }
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedElement(element, child);
                            break;
                    }
                }
            }
        }

        private void ParseExecOnComponentElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string binary = null;
            string command = null;
            string workDir = null;
            ExecOnComponentFlags flags = ExecOnComponentFlags.None;
            ErrorHandling errorHandling = ErrorHandling.fail;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            YesNoType aye;
            string user = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                switch (attrib.Name.LocalName)
                {
                    case "Command":
                        command = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "BinaryKey":
                        binary = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "WorkingDirectory":
                        workDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "Order":
                        order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                        if (order < 0)
                        {
                            order += int.MaxValue;
                        }
                        break;

                    case "Impersonate":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.Impersonate;
                        }
                        break;

                    case "User":
                        user = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                        break;

                    case "ErrorHandling":
                        TryParseEnumAttribute(sourceLineNumbers, element, attrib, out errorHandling);
                        break;

                    case "Wait":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.No)
                        {
                            flags |= ExecOnComponentFlags.ASync;
                        }
                        break;

                    case "OnInstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnInstall;
                        }
                        break;

                    case "OnInstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnInstallRollback;
                        }
                        break;

                    case "OnReinstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnReinstall;
                        }
                        break;

                    case "OnReinstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnReinstallRollback;
                        }
                        break;

                    case "OnUninstall":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnRemove;
                        }
                        break;

                    case "OnUninstallRollback":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.OnRemoveRollback;
                        }
                        break;

                    case "BeforeStopServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.BeforeStopServices;
                        }
                        break;

                    case "AfterStopServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.AfterStopServices;
                        }
                        break;

                    case "BeforeStartServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.BeforeStartServices;
                        }
                        break;

                    case "AfterStartServices":
                        aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                        if (aye == YesNoType.Yes)
                        {
                            flags |= ExecOnComponentFlags.AfterStartServices;
                        }
                        break;

                    default:
                        ParseHelper.UnexpectedAttribute(element, attrib);
                        break;
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(command))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Command"));
            }
            if ((flags & ExecOnComponentFlags.AnyAction) == ExecOnComponentFlags.None)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "OnXXX"));
            }
            if ((flags & ExecOnComponentFlags.AnyTiming) == ExecOnComponentFlags.None)
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "BeforeXXX or AfterXXX"));
            }
            if ((flags & ExecOnComponentFlags.ASync) == ExecOnComponentFlags.ASync)
            {
                errorHandling = ErrorHandling.ignore;
            }


            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ExecOnComponent");
            PSW_ExecOnComponent row = null;
            if (!Messaging.EncounteredError)
            {
                // Ensure sub-tables exist for queries to succeed even if no sub-entries exist.
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_ExecOnComponent_ExitCode");
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_ExecOn_ConsoleOutput");
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_ExecOnComponent_Environment");
                row = section.AddSymbol(new PSW_ExecOnComponent(sourceLineNumbers));
                row.Component_ = component;
                row.Binary_ = binary;
                row.Command = command;
                row.WorkingDirectory = workDir;
                row.Flags = (int)flags;
                row.ErrorHandling = (int)errorHandling;
                row.Order = order;
                row.User_ = user;
            }

            // ExitCode mapping
            foreach (XElement child in element.Descendants())
            {
                if (child.Name.Namespace.Equals(Namespace))
                {
                    switch (child.Name.LocalName)
                    {
                        case "ExitCode":
                            {
                                ushort from = 0, to = 0;
                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (IsMyAttribute(child, a))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Value":
                                                from = (ushort)ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, a, 0, 0xffff);
                                                break;

                                            case "Behavior":
                                                switch (ParseHelper.GetAttributeValue(sourceLineNumbers, a))
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
                                                        to = (ushort)ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, a, 0, 0xffff);
                                                        break;
                                                }
                                                break;
                                        }
                                    }
                                }
                                if (CheckNoCData(child) && !Messaging.EncounteredError)
                                {
                                    PSW_ExecOnComponent_ExitCode row1 = section.AddSymbol(new PSW_ExecOnComponent_ExitCode(sourceLineNumbers, row.Id));
                                    row1.From = from;
                                    row1.To = to;
                                }
                            }
                            break;

                        case "ConsoleOutput":
                            {
                                ErrorHandling stdoutHandling = ErrorHandling.fail;
                                string regex = null;
                                string prompt = null;
                                bool onMatch = true;

                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (IsMyAttribute(child, a))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Expression":
                                                regex = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            case "BehaviorOnMatch":
                                                onMatch = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, a) == YesNoType.Yes);
                                                break;

                                            case "PromptText":
                                                prompt = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            case "Behavior":
                                                TryParseEnumAttribute(sourceLineNumbers, child, a, out stdoutHandling);
                                                break;
                                        }
                                    }
                                }
                                if ((stdoutHandling == ErrorHandling.prompt) == string.IsNullOrEmpty(prompt))
                                {
                                    Messaging.Write(ErrorMessages.IllegalAttributeValueWithOtherAttribute(sourceLineNumbers, child.Name.LocalName, "Behavior", "prompt", "PromptText"));
                                }
                                if (string.IsNullOrEmpty(regex))
                                {
                                    Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, child.Name.LocalName, "Expression"));
                                }

                                if (CheckNoCData(child) && !Messaging.EncounteredError)
                                {
                                    PSW_ExecOn_ConsoleOutput row1 = section.AddSymbol(new PSW_ExecOn_ConsoleOutput(sourceLineNumbers, row.Id));
                                    row1.Expression = regex;
                                    row1.Flags = onMatch ? 1 : 0;
                                    row1.ErrorHandling = (int)stdoutHandling;
                                    row1.PromptText = prompt;
                                }
                            }
                            break;


                        case "Environment":
                            {
                                string name = null, value = null;

                                foreach (XAttribute a in child.Attributes())
                                {
                                    if (IsMyAttribute(child, a))
                                    {
                                        switch (a.Name.LocalName)
                                        {
                                            case "Value":
                                                value = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            case "Name":
                                                name = ParseHelper.GetAttributeValue(sourceLineNumbers, a);
                                                break;

                                            default:
                                                ParseHelper.UnexpectedAttribute(child, a);
                                                break;
                                        }
                                    }
                                }
                                if (string.IsNullOrEmpty(name) || string.IsNullOrEmpty(value))
                                {
                                    Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, child.Name.LocalName, "Name", "Value"));
                                    break;
                                }

                                if (CheckNoCData(child) && !Messaging.EncounteredError)
                                {
                                    PSW_ExecOnComponent_Environment row1 = section.AddSymbol(new PSW_ExecOnComponent_Environment(sourceLineNumbers, row.Id));
                                    row1.Name = name;
                                    row1.Value = value;
                                }
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedElement(element, child);
                            break;
                    }
                }
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

        private void ParseServiceConfigElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string service = null;
            string commandLine = null;
            string account = null;
            string password = null;
            string loadOrderGroup = null;
            ServiceStart start = ServiceStart.unchanged;
            ErrorHandling errorHandling = ErrorHandling.fail;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "ServiceName":
                            service = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "CommandLine":
                            commandLine = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Account":
                            account = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Start":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out start);
                            break;

                        case "LoadOrderGroup":
                            loadOrderGroup = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out errorHandling);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(service))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "ServiceName"));
            }
            if (string.IsNullOrEmpty(account) && !string.IsNullOrEmpty(password))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributesWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "Password", "Account"));
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_ServiceConfig");

            PSW_ServiceConfig mainRow = null;
            if (!Messaging.EncounteredError)
            {
                // Ensure sub-table exists for queries to succeed even if no sub-entries exist.
                ParseHelper.EnsureTable(section, sourceLineNumbers, "PSW_ServiceConfig_Dependency");
                mainRow = section.AddSymbol(new PSW_ServiceConfig(sourceLineNumbers));
                mainRow.Component_ = component;
                mainRow.ServiceName = service;
                mainRow.CommandLine = commandLine;
                mainRow.Account = account;
                mainRow.Password = password;
                mainRow.Start = (start == ServiceStart.autoDelayed) ? (short)ServiceStart.auto : (short)start;
                mainRow.DelayStart = (start == ServiceStart.autoDelayed) ? (short)1 : (start == ServiceStart.auto) ? (short)0 : (short)-1;
                mainRow.LoadOrderGroup = loadOrderGroup;
                mainRow.ErrorHandling = (short)errorHandling;
            }

            foreach (XElement child in element.Descendants())
            {
                if (child.Name.Namespace.Equals(Namespace))
                {
                    if (!child.Name.LocalName.Equals("Dependency"))
                    {
                        ParseHelper.UnexpectedElement(element, child);
                        continue;
                    }

                    sourceLineNumbers = ParseHelper.GetSourceLineNumbers(child);
                    foreach (XAttribute attrib in child.Attributes())
                    {
                        if (IsMyAttribute(element, attrib))
                        {
                            string depService = null;
                            string group = null;

                            switch (attrib.Name.LocalName)
                            {
                                case "Service":
                                    depService = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                    break;

                                case "Group":
                                    group = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                                    break;

                                default:
                                    ParseHelper.UnexpectedAttribute(child, attrib);
                                    break;
                            }

                            if (string.IsNullOrEmpty(depService) && string.IsNullOrEmpty(group))
                            {
                                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, child.Name.LocalName, "Service", "Group"));
                            }

                            if (CheckNoCData(child) && !Messaging.EncounteredError)
                            {
                                PSW_ServiceConfig_Dependency row = section.AddSymbol(new PSW_ServiceConfig_Dependency(sourceLineNumbers, mainRow.Id));
                                row.Service = depService;
                                row.Group = group;
                            }
                        }
                    }
                }
            }
        }

        private void ParseDismElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string features = null;
            string exclude = null;
            string package = null;
            string unwanted = null;
            ErrorHandling promptOnError = ErrorHandling.fail;
            int cost = 20971520; // 20 MB.
            int enableAll = -1;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            int forceRemove = -1;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "EnableFeature":
                            features = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "ExcludeFeatures":
                            exclude = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "PackagePath":
                            package = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Cost":
                            cost = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, int.MaxValue);
                            break;

                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out promptOnError);
                            break;

                        case "EnableAll":
                            enableAll = (int)ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            break;

                        case "RemoveFeature":
                            unwanted = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "ForceRemove":
                            forceRemove = (int)ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            break;

                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrEmpty(features) && string.IsNullOrEmpty(package))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "EnableFeature", "PackagePath"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "DismSched");
                PSW_Dism row = section.AddSymbol(new PSW_Dism(sourceLineNumbers));
                row.Component_ = component;
                row.EnableFeatures = features;
                row.ExcludeFeatures = exclude;
                row.RemoveFeatures = unwanted;
                row.PackagePath = package;
                row.Cost = cost;
                row.ErrorHandling = (int)promptOnError;
                row.EnableAll = enableAll;
                row.ForceRemove = forceRemove;
                row.Order = order;
            }
        }

        private void ParseTaskSchedulerElement(IntermediateSection section, XElement element, string component)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string taskXml = null;
            string taskName = null;
            string user = null;
            string password = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "TaskName":
                            taskName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "User":
                            user = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "XmlFile":
                            taskXml = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            foreach (XElement child in element.Descendants())
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.Name.Namespace.Equals(Namespace))
                    {
                        ParseHelper.UnexpectedElement(element, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    if (!string.IsNullOrWhiteSpace(taskXml))
                    {
                        Messaging.Write(ErrorMessages.IllegalAttributeWithInnerText(sourceLineNumbers, element.Name.LocalName, "XmlFile"));
                    }
                    taskXml = child.Value;
                }
            }
            if (!string.IsNullOrEmpty(element.Value))
            {
                if (!string.IsNullOrWhiteSpace(taskXml))
                {
                    Messaging.Write(ErrorMessages.IllegalAttributeWithInnerText(sourceLineNumbers, element.Name.LocalName, "XmlFile"));
                }
                taskXml = element.Value;
            }

            if (string.IsNullOrEmpty(component))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, "Component", "Id"));
            }
            if (string.IsNullOrWhiteSpace(taskXml))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributeOrElement(sourceLineNumbers, element.Name.LocalName, "XmlFile", "Inner text or CDATA"));
            }
            if (string.IsNullOrEmpty(taskName))
            {
                Messaging.Write(ErrorMessages.ExpectedElement(sourceLineNumbers, element.Name.LocalName, "TaskName"));
            }
            if (string.IsNullOrEmpty(user) && !string.IsNullOrEmpty(password))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributesWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "User", "Password"));
            }

            if (!Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "TaskScheduler");

                taskXml = taskXml.Trim();
                taskXml = taskXml.Replace("\r", "");
                taskXml = taskXml.Replace("\n", "");
                taskXml = taskXml.Replace(Environment.NewLine, "");

                PSW_TaskScheduler row = section.AddSymbol(new PSW_TaskScheduler(sourceLineNumbers));
                row.TaskName = taskName;
                row.Component_ = component;
                row.TaskXml = taskXml;
                row.User = user;
                row.Password = password;
            }
        }

        [Flags]
        private enum CustomUninstallKeyAttributes
        {
            write = 1,
            delete = 2
        }

        private void ParseCustomUninstallKeyElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string productCode = null;
            string name = null;
            string data = null;
            string datatype = "REG_SZ";
            string condition = null;
            CustomUninstallKeyAttributes attributes = CustomUninstallKeyAttributes.write;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "ProductCode":
                            productCode = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Name":
                            name = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Data":
                            data = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "DataType":
                            datatype = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Operation": //TODO Isn't documented in the XSD
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out attributes);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(name))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Name"));
            }

            if (string.IsNullOrEmpty(data))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Data"));
            }

            if (string.IsNullOrEmpty(datatype))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "DataType"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CustomUninstallKey_Immediate");
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CustomUninstallKey_deferred");
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CustomUninstallKey_rollback");

                PSW_CustomUninstallKey row = section.AddSymbol(new PSW_CustomUninstallKey(sourceLineNumbers));
                row.ProductCode = productCode;
                row.Name = name;
                row.Data = data;
                row.DataType = datatype;
                row.Attributes = (int)attributes;
                row.Condition = condition;
            }
        }

        private enum ReadIniValuesAttributes
        {
            None = 0,
            IgnoreErrors = 1
        }

        private void ParseReadIniValuesElement(IntermediateSection section, XElement element, XElement parent)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string FilePath = null;
            string Section = null;
            string Key = null;
            YesNoType IgnoreErrors = YesNoType.No;
            string condition = null;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "FilePath":
                            FilePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Section":
                            Section = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Key":
                            Key = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "IgnoreErrors":
                            IgnoreErrors = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(FilePath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "FilePath"));
            }

            if (string.IsNullOrEmpty(Key))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Key"));
            }

            if (string.IsNullOrEmpty(Section))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Section"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ReadIniValues");
                PSW_ReadIniValues row = section.AddSymbol(new PSW_ReadIniValues(sourceLineNumbers));
                row.FilePath = FilePath;
                row.Section = Section;
                row.Key = Key;
                row.DestProperty = property.Id;
                row.Attributes = (IgnoreErrors == YesNoType.Yes) ? 1 : 0;
                row.Condition = condition;
            }
        }

        private enum RegistryArea
        {
            x86,
            x64,
            Default
        }

        private void ParseRemoveRegistryValue(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string root = null;
            string key = null;
            string name = null;
            RegistryArea area = RegistryArea.Default;
            string condition = "";

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Root":
                            root = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Key":
                            key = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Name":
                            name = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Area":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out area);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(key))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Key"));
            }
            if (string.IsNullOrEmpty(root))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Root"));
            }
            if (string.IsNullOrEmpty(name))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Name"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "RemoveRegistryValue_Immediate");
                PSW_RemoveRegistryValue row = section.AddSymbol(new PSW_RemoveRegistryValue(sourceLineNumbers));
                row.Root = root;
                row.Key = key;
                row.Name = name;
                row.Area = area.ToString();
                row.Attributes = 0;
                row.Condition = condition;
            }
        }

        private void ParseCertificateHashSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string property = null;
            string certName = null;
            string friendlyName = null;
            string issuer = null;
            string serial = null;

            if (element.Parent.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(element.Parent, element);
            }
            property = element.Parent.Attribute("Id").Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "CertName":
                            certName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "FriendlyName":
                            friendlyName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Issuer":
                            issuer = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "SerialNumber":
                            serial = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Parent.Name.LocalName, "Id"));
            }

            // At least CertName OR (Issuer AND Serial) OR FriendlyName
            if (string.IsNullOrEmpty(issuer) != string.IsNullOrEmpty(serial))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "Issuer", "SerialNumber"));
            }
            if (string.IsNullOrEmpty(certName) && string.IsNullOrEmpty(issuer) && string.IsNullOrEmpty(friendlyName))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "Issuer", "SerialNumber", "CertName", "FriendlyName"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "CertificateHashSearch");

                PSW_CertificateHashSearch row = section.AddSymbol(new PSW_CertificateHashSearch(sourceLineNumbers, property));
                row.CertName = certName;
                row.FriendlyName = friendlyName;
                row.Issuer = issuer;
                row.SerialNumber = serial;
            }
        }

        private void ParseEvaluateElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string property = null;
            string expression = null;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;

            if (element.Parent.Name.LocalName != "Property")
            {
                ParseHelper.UnexpectedElement(element.Parent, element);
            }
            property = element.Parent.Attribute("Id").Value;
            if (!property.ToUpper().Equals(property))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property));
            }

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Expression":
                            expression = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Parent.Name.LocalName, "Id"));
            }
            if (string.IsNullOrEmpty(expression))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Expression"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "EvaluateExpression");

                PSW_EvaluateExpression row = section.AddSymbol(new PSW_EvaluateExpression(sourceLineNumbers));
                row.Property_ = property;
                row.Expression = expression;
                row.Order = order;
            }
        }

        private void ParsePathSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string file = null;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "FileName":
                            file = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(file))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "FileName"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_PathSearch");

                PSW_PathSearch row = section.AddSymbol(new PSW_PathSearch(sourceLineNumbers));
                row.Property_ = property.Id;
                row.FileName = file;
            }
        }

        private void ParseVersionCompareElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string version1 = null;
            string version2 = null;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Version1":
                            version1 = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Version2":
                            version2 = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(version1))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Version1"));
            }
            if (string.IsNullOrEmpty(version2))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Version2"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_VersionCompare");

                PSW_VersionCompare row = section.AddSymbol(new PSW_VersionCompare(sourceLineNumbers));
                row.Property_ = property.Id;
                row.Version1 = version1;
                row.Version2 = version2;
            }
        }

        private void ParseAccountSidSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string systemName = null;
            string accountName = null;
            string condition = "";
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "AccountName":
                            accountName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "SystemName":
                            systemName = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(accountName))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "AccountName"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "AccountSidSearch");

                PSW_AccountSidSearch row = section.AddSymbol(new PSW_AccountSidSearch(sourceLineNumbers));
                row.Property_ = property.Id;
                row.SystemName = systemName;
                row.AccountName = accountName;
                row.Condition = condition;
            }
        }

        private enum XmlSearchMatch
        {
            first,
            all,
            enforceSingle
        }

        private void ParseXmlSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string filePath = null;
            string xpath = null;
            string lang = null;
            string namespaces = null;
            XmlSearchMatch match = XmlSearchMatch.first;
            string condition = null;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "FilePath":
                            filePath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "XPath":
                            xpath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Language":
                            lang = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Namespaces":
                            namespaces = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Match":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out match);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(filePath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "FilePath"));
            }
            if (string.IsNullOrEmpty(xpath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "XPath"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "XmlSearch");

                PSW_XmlSearch row = section.AddSymbol(new PSW_XmlSearch(sourceLineNumbers));
                row.Property_ = property.Id;
                row.FilePath = filePath;
                row.Expression = xpath;
                row.Language = lang;
                row.Namespaces = namespaces;
                row.Match = (int)match;
                row.Condition = condition;
            }
        }

        private void ParseSqlSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
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
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "ConnectionString":
                            connectionString = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Server":
                            server = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Instance":
                            instance = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Database":
                            database = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Username":
                            username = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Password":
                            password = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;
                        case "Port":
                            port = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Encrypt":
                            encrypted = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ErrorHandling":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out errorHandling);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (!string.IsNullOrEmpty(connectionString) &&
                (!string.IsNullOrEmpty(server) || !string.IsNullOrEmpty(instance) || !string.IsNullOrEmpty(database) || !string.IsNullOrEmpty(port) || !string.IsNullOrEmpty(encrypted) || !string.IsNullOrEmpty(password) || !string.IsNullOrEmpty(username)))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "ConnectionString", "any other"));
            }
            if (string.IsNullOrEmpty(server) && string.IsNullOrEmpty(connectionString))
            {
                Messaging.Write(ErrorMessages.ExpectedAttributes(sourceLineNumbers, element.Name.LocalName, "Server", "ConnectionString"));
            }
            if (string.IsNullOrEmpty(query))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Query"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "SqlSearch");

                PSW_SqlSearch row = section.AddSymbol(new PSW_SqlSearch(sourceLineNumbers));
                row.Property_ = property.Id;
                row.Server = server;
                row.Instance = instance;
                row.Port = port;
                row.Encrypted = encrypted;
                row.Database = database;
                row.Username = username;
                row.Password = password;
                row.Query = query;
                row.Condition = condition;
                row.Order = order;
                row.ErrorHandling = (int)errorHandling;
                row.ConnectionString = connectionString;
            }
        }

        private void ParseWmiSearchElement(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string nmspace = null;
            string query = null;
            string resultProp = null;
            string condition = null;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Namespace":
                            nmspace = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Query":
                            query = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ResultProperty":
                            resultProp = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(query))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Query"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "WmiSearch");

                PSW_WmiSearch row = section.AddSymbol(new PSW_WmiSearch(sourceLineNumbers));
                row.Property_ = property.Id;
                row.Condition = condition;
                row.Namespace = nmspace;
                row.Query = query;
                row.ResultProperty = resultProp;
                row.Order = order;
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

        private void ParseTelemetry(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string url = null;
            string page = null;
            string method = null;
            string data = null;
            ExecutePhase flags = ExecutePhase.None;
            string condition = "";

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Url":
                            url = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Page":
                            page = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Method":
                            method = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Data":
                            data = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "OnSuccess":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "OnStart":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "OnFailure":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;
                        case "Secure":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.Secure;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(url))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Url"));
            }
            if (string.IsNullOrEmpty(method))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Method"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "Telemetry");

                PSW_Telemetry row = section.AddSymbol(new PSW_Telemetry(sourceLineNumbers));
                row.Url = url;
                row.Page = page ?? "";
                row.Method = method;
                row.Data = data ?? "";
                row.Flags = (int)flags;
                row.Condition = condition;
            }
        }

        private void ParseShellExecute(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string target = null;
            string args = "";
            string workDir = "";
            string verb = "";
            int wait = 0;
            int show = 0;
            ExecutePhase flags = ExecutePhase.None;
            string condition = "";

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Target":
                            target = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Args":
                            args = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "WorkingDir":
                            workDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Verb":
                            verb = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Wait":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                wait = 1;
                            }
                            break;
                        case "Show":
                            show = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 15);
                            break;

                        case "OnCommit":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnCommit;
                            }
                            break;
                        case "OnExecute":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnExecute;
                            }
                            break;
                        case "OnRollback":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= ExecutePhase.OnRollback;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(target))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Target"));
            }

            // Default to execute deferred.
            if (flags == ExecutePhase.None)
            {
                flags = ExecutePhase.OnExecute;
            }

            ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ShellExecute_Immediate");

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                PSW_ShellExecute row = section.AddSymbol(new PSW_ShellExecute(sourceLineNumbers));
                row.Target = target;
                row.Args = args;
                row.Verb = verb;
                row.WorkingDir = workDir;
                row.Show = show;
                row.Wait = wait;
                row.Flags = (int)flags;
                row.Condition = condition;
            }
        }

        private void ParseMsiSqlQuery(IntermediateSection section, XElement element, XElement parent)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string query = null;
            string condition = null;
            Identifier property = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Query":
                            query = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(query))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Query"));
            }
            if (query.StartsWith("select", StringComparison.OrdinalIgnoreCase))
            {
                TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "MsiSqlQuery");

                PSW_MsiSqlQuery row = section.AddSymbol(new PSW_MsiSqlQuery(sourceLineNumbers));
                row.Property_ = property?.Id;
                row.Query = query;
                row.Condition = condition;
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

        private void ParseRegularExpression(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string filepath = null;
            string input = null;
            string regex = null;
            string replacement = null;
            int flags = 0;
            string condition = null;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;
            Identifier property = null;
            TryGetParentSearchPropertyId(sourceLineNumbers, element, out property);

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "FilePath":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Input":
                            input = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Expression":
                            regex = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Replacement":
                            replacement = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib, EmptyRule.CanBeEmpty);
                            flags |= (int)RegexSearchFlags.Replace;
                            break;
                        case "IgnoreCase":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= (int)RegexMatchFlags.IgnoreCare << 2;
                            }
                            break;
                        case "Extended":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= (int)RegexMatchFlags.Extended << 2;
                            }
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(regex))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Expression"));
            }
            if (string.IsNullOrEmpty(input) == string.IsNullOrEmpty(filepath))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeWithOtherAttribute(sourceLineNumbers, element.Name.LocalName, "Input", "FilePath"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "RegularExpression");

                PSW_RegularExpression row = section.AddSymbol(new PSW_RegularExpression(sourceLineNumbers));
                row.FilePath = filepath;
                row.Input = input;
                row.Expression = regex;
                row.Replacement = replacement;
                row.DstProperty_ = property.Id;
                row.Flags = flags;
                row.Condition = condition;
                row.Order = order;
            }
        }

        private void ParseRestartLocalResources(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string path = null;
            string condition = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Path":
                            path = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(path))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Path"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "RestartLocalResources");

                PSW_RestartLocalResources row = section.AddSymbol(new PSW_RestartLocalResources(sourceLineNumbers));
                row.Path = path;
                row.Condition = condition;
            }
        }

        private enum FileEncoding
        {
            AutoDetect,
            MultiByte,
            Unicode,
            ReverseUnicode
        };

        private void ParseFileRegex(IntermediateSection section, XElement element, string component, string fileId)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string filepath = null;
            string condition = null;
            string regex = null;
            string replacement = null;
            FileEncoding encoding = FileEncoding.AutoDetect;
            bool ignoreCase = false;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "FilePath":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Regex":
                            regex = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Replacement":
                            replacement = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib, EmptyRule.CanBeEmpty);
                            break;
                        case "IgnoreCase":
                            ignoreCase = true;
                            break;
                        case "Encoding":
                            TryParseEnumAttribute(sourceLineNumbers, element, attrib, out encoding);
                            break;
                        case "Order":
                            order = ParseHelper.GetAttributeIntegerValue(sourceLineNumbers, attrib, -1000000000, 1000000000);
                            if (order < 0)
                            {
                                order += int.MaxValue;
                            }
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(regex))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Regex"));
            }
            if (string.IsNullOrEmpty(fileId) == string.IsNullOrEmpty(filepath))
            {
                // Either under File or specify FilePath, not both
                Messaging.Write(ErrorMessages.IllegalAttributeWhenNested(sourceLineNumbers, element.Name.LocalName, "FilePath", "Package"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "FileRegex_Immediate");

                PSW_FileRegex row = section.AddSymbol(new PSW_FileRegex(sourceLineNumbers));
                row.Component_ = component;
                row.File_ = fileId;
                row.FilePath = filepath;
                row.Regex = regex;
                row.Replacement = replacement ?? "";
                row.IgnoreCase = ignoreCase ? 1 : 0;
                row.Encoding = (int)encoding;
                row.Condition = condition;
                row.Order = order;
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

        private void ParseDeletePath(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string filepath = null;
            string condition = null;
            DeletePathFlags flags = DeletePathFlags.AllowReboot | DeletePathFlags.IgnoreErrors | DeletePathFlags.IgnoreMissing;
            int order = 1000000000 + sourceLineNumbers.LineNumber ?? 0;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "Path":
                            filepath = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "OnlyIfEmpty":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes)
                            {
                                flags |= DeletePathFlags.OnlyIfEmpty;
                            }
                            break;
                        case "AllowReboot":
                            if (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.No)
                            {
                                flags &= ~DeletePathFlags.AllowReboot;
                            }
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(filepath))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "Path"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                if (flags.HasFlag(DeletePathFlags.AllowReboot))
                {
                    ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "PSW_CheckRebootRequired");
                }
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "DeletePath");

                PSW_DeletePath row = section.AddSymbol(new PSW_DeletePath(sourceLineNumbers));
                row.Path = filepath;
                row.Flags = (int)flags;
                row.Condition = condition;
                row.Order = order;
            }
        }

        private void ParseZipFile(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string dstZipFile = null;
            string srcDir = null;
            string filePattern = "*.*";
            bool recursive = true;
            string condition = null;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "TargetZipFile":
                            dstZipFile = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "SourceFolder":
                            srcDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "FilePattern":
                            filePattern = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "Recursive":
                            recursive = (ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib) == YesNoType.Yes);
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(dstZipFile))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "TargetZipFile"));
            }
            if (string.IsNullOrEmpty(srcDir))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "SourceFolder"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "ZipFileSched");

                PSW_ZipFile row = section.AddSymbol(new PSW_ZipFile(sourceLineNumbers));
                row.ZipFile = dstZipFile;
                row.CompressFolder = srcDir;
                row.FilePattern = filePattern;
                row.Recursive = recursive ? 1 : 0;
                row.Condition = condition;
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

        private void ParseUnzip(IntermediateSection section, XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            string zipFile = null;
            string dstDir = null;
            string condition = null;
            UnzipFlags flags = UnzipFlags.Unmodified | UnzipFlags.CreateRoot;

            foreach (XAttribute attrib in element.Attributes())
            {
                if (IsMyAttribute(element, attrib))
                {
                    switch (attrib.Name.LocalName)
                    {
                        case "ZipFile":
                            zipFile = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "TargetFolder":
                            dstDir = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "CreateRootFolder":
                            {
                                YesNoType aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.No)
                                {
                                    flags &= ~UnzipFlags.CreateRoot;
                                }
                            }
                            break;
                        case "DeleteZip":
                            {
                                YesNoType aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.Yes)
                                {
                                    flags |= UnzipFlags.Delete;
                                }
                            }
                            break;
                        case "OnRollback":
                            {
                                YesNoType aye = ParseHelper.GetAttributeYesNoValue(sourceLineNumbers, attrib);
                                if (aye == YesNoType.Yes)
                                {
                                    flags |= UnzipFlags.OnRollback;
                                }
                            }
                            break;
                        case "Overwrite":
                            {
                                YesNoDefaultType aye = ParseHelper.GetAttributeYesNoDefaultValue(sourceLineNumbers, attrib);
                                flags = ((aye == YesNoDefaultType.Yes) ? ((flags & ~UnzipFlags.OverwriteMask) | UnzipFlags.Always)
                                    : (aye == YesNoDefaultType.No) ? (flags & ~UnzipFlags.OverwriteMask)
                                    : ((flags & ~UnzipFlags.OverwriteMask) | UnzipFlags.Unmodified));
                            }
                            break;
                        case "OverwriteMode":
                            if (TryParseEnumAttribute(sourceLineNumbers, element, attrib, out UnzipFlags f))
                            {
                                flags = ((flags & ~UnzipFlags.OverwriteMask) | f);
                            }
                            break;
                        case "Condition":
                            condition = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            ParseHelper.UnexpectedAttribute(element, attrib);
                            break;
                    }
                }
            }

            if (string.IsNullOrEmpty(zipFile))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "ZipFile"));
            }
            if (string.IsNullOrEmpty(dstDir))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, element.Name.LocalName, "TargetFolder"));
            }

            if (CheckNoCData(element) && !Messaging.EncounteredError)
            {
                ParseHelper.CreateSimpleReference(section, sourceLineNumbers, "CustomAction", "UnzipSched");

                PSW_Unzip row = section.AddSymbol(new PSW_Unzip(sourceLineNumbers));
                row.ZipFile = zipFile;
                row.TargetFolder = dstDir;
                row.Flags = (int)flags;
                row.Condition = condition;
            }
        }

        private bool TryParseEnumAttribute<T>(SourceLineNumber sourceLineNumbers, XElement element, XAttribute attrib, out T value) where T : struct
        {
            value = default(T);
            string v = ParseHelper.GetAttributeValue(sourceLineNumbers, attrib);
            if (string.IsNullOrEmpty(v) || !Enum.TryParse<T>(v, true, out value))
            {
                Messaging.Write(ErrorMessages.IllegalAttributeValue(sourceLineNumbers, element.Name.LocalName, attrib.Name.LocalName, v));
                return false;
            }
            return true;
        }

        private bool TryGetParentSearchPropertyId(SourceLineNumber sourceLineNumbers, XElement child, out Identifier property)
        {
            property = null;

            if (!child.Parent.Name.LocalName.Equals("Property"))
            {
                ParseHelper.UnexpectedElement(child.Parent, child);
                return false;
            }

            property = ParseHelper.GetAttributeIdentifier(sourceLineNumbers, child.Parent.Attribute("Id"));
            if ((property == null) || string.IsNullOrEmpty(property.Id))
            {
                Messaging.Write(ErrorMessages.ExpectedAttribute(sourceLineNumbers, child.Parent.Name.LocalName, "Id"));
                return false;
            }
            if (!property.Id.ToUpper().Equals(property.Id))
            {
                Messaging.Write(ErrorMessages.SearchPropertyNotUppercase(sourceLineNumbers, "Property", "Id", property.Id));
                return false;
            }
            return true;
        }

        private bool IsMyAttribute(XElement element, XAttribute attrib)
        {
            return ((element.Name.Namespace.Equals(Namespace) && string.IsNullOrEmpty(attrib.Name.NamespaceName)) || attrib.Name.Namespace.Equals(Namespace));
        }

        private bool CheckNoCData(XElement element)
        {
            SourceLineNumber sourceLineNumbers = ParseHelper.GetSourceLineNumbers(element);
            foreach (XElement child in element.Descendants())
            {
                if ((child.NodeType == XmlNodeType.Text) || (child.NodeType == XmlNodeType.CDATA))
                {
                    Messaging.Write(ErrorMessages.IllegalInnerText(sourceLineNumbers, element.Name.LocalName, child.Value));
                    return false;
                }
            }
            if (!string.IsNullOrEmpty(element.Value))
            {
                Messaging.Write(ErrorMessages.IllegalInnerText(sourceLineNumbers, element.Name.LocalName, element.Value));
                return false;
            }
            return true;
        }

        public override void PostCompile(Intermediate intermediate)
        {
            base.PostCompile(intermediate);
            SplitFiles(intermediate);
        }

        private void SplitFiles(Intermediate intermediate)
        {
            // This section is empty, need to iterate the other sections
            List<PSW_ConcatFiles> concatFiles = new List<PSW_ConcatFiles>();
            List<FileSymbol> allFiles = new List<FileSymbol>();
            foreach (IntermediateSection section in intermediate.Sections)
            {
                foreach (IntermediateSymbol symbol in section.Symbols)
                {
                    if (symbol is PSW_ConcatFiles concatSymbol)
                    {
                        concatFiles.Add(concatSymbol);
                    }
                    else if (symbol is FileSymbol f)
                    {
                        allFiles.Add(f);
                    }
                }
            }

            concatFiles.Sort(new ConcatFilesComparer());

            string tmpPath = Path.GetTempPath();
            FileSymbol rootWixFile = null;
            int splitSize = Int32.MaxValue;
            FileStream rootFileStream = null;
            try
            {
                foreach (PSW_ConcatFiles concatSymbol in concatFiles)
                {
                    // New root file
                    if (!concatSymbol.RootFile_.Equals(rootWixFile?.Id?.Id))
                    {
                        splitSize = concatSymbol.Size;
                        rootWixFile = allFiles.FirstOrDefault(f => f.Id.Id.Equals(concatSymbol.RootFile_));
                        if (rootWixFile == null)
                        {
                            Messaging.Write(ErrorMessages.WixFileNotFound(concatSymbol.RootFile_));
                            return;
                        }

                        rootFileStream?.Dispose();
                        rootFileStream = null; // Ensure no double-dispose in case next line throws
                        rootFileStream = File.OpenRead(rootWixFile.Source.Path);

                        string splId = "spl" + Guid.NewGuid().ToString("N");
                        rootWixFile.Source.Path = Path.Combine(tmpPath, splId);
                        CopyFilePart(rootFileStream, rootWixFile.Source.Path, splitSize);
                    }

                    FileSymbol currWixFile = allFiles.FirstOrDefault(f => f.Id.Id.Equals(concatSymbol.MyFile_));
                    if (currWixFile == null)
                    {
                        Messaging.Write(ErrorMessages.WixFileNotFound(concatSymbol.MyFile_));
                        return;
                    }

                    CopyFilePart(rootFileStream, currWixFile.Source.Path, splitSize);
                }
            }
            finally
            {
                rootFileStream?.Dispose();
            }
        }

        private void CopyFilePart(FileStream srcFile, string dstFile, int copySize)
        {
            byte[] buffer = new byte[1024 * 1024]; // 1MB chunks
            long tmpFileSize = 0;
            using (FileStream dstFileStream = File.OpenWrite(dstFile))
            {
                while ((tmpFileSize < copySize) && (srcFile.Position < srcFile.Length))
                {
                    int chunkSize = (int)Math.Min(copySize - tmpFileSize, buffer.Length);
                    chunkSize = srcFile.Read(buffer, 0, chunkSize);
                    dstFileStream.Write(buffer, 0, chunkSize);
                    tmpFileSize += chunkSize;
                }
            }
        }
    }

    class ConcatFilesComparer : IComparer<PSW_ConcatFiles>
    {
        int IComparer<PSW_ConcatFiles>.Compare(PSW_ConcatFiles x, PSW_ConcatFiles y)
        {
            int rootFileCmp = x.RootFile_.CompareTo(y.RootFile_);
            if (rootFileCmp != 0)
            {
                return rootFileCmp;
            }

            // Same file; Compare by order
            return x.Order.CompareTo(y.Order);
        }
    }
}
