namespace PanelSw.Wix.Extensions
{
    using System;
    using System.Collections;
    using System.Globalization;
    using System.Reflection;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Xml;
    using System.Xml.Schema;

    using Microsoft.Tools.WindowsInstallerXml;

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
            this.schema = LoadXmlSchemaHelper(Assembly.GetExecutingAssembly(), "PanelSw.Wix.Extensions.Xsd.PanelSwWixExtension.xsd");
        }

        /// <summary>
        /// Gets the schema for this extension.
        /// </summary>
        /// <value>Schema for this extension.</value>
        public override XmlSchema Schema
        {
            get { return this.schema; }
        }

        public override void FinalizeCompile()
        {
            Core.EnsureTable(null, "PSW_CustomUninstallKey");
            Core.EnsureTable(null, "PSW_ReadIniValues");
            Core.EnsureTable(null, "PSW_RemoveRegistryValue");
            Core.EnsureTable(null, "PSW_XmlSearch");
            Core.EnsureTable(null, "PSW_Telemetry");
            Core.EnsureTable(null, "PSW_ShellExecute");
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
                            this.ParseCustomUninstallKeyElement(element);
                            break;

                        case "ReadIniValues":
                            this.ParseReadIniValuesElement(element);
                            break;

                        case "RemoveRegistryValue":
                            this.ParseRemoveRegistryValue(element);
                            break;

                        case "Telemetry":
                            this.ParseTelemetry(element);
                            break;

                        case "ShellExecute":
                            this.ParseShellExecute(element);
                            break;

                        default:
                            this.Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                case "Property":
                    switch (element.LocalName)
                    {
                        case "XmlSearch":
                            this.ParseXmlSearchElement(element);
                            break;

                        default:
                            this.Core.UnexpectedElement(parentElement, element);
                            break;
                    }
                    break;

                default:
                    this.Core.UnexpectedElement(parentElement, element);
                    break;
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
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == this.schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (string.IsNullOrEmpty(name))
                            {
                                name = id;
                            }
                            break;
                        case "name":
                            name = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "data":
                            data = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "datatype":
                            datatype = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "operation":
                            if (this.Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("delete", StringComparison.OrdinalIgnoreCase))
                            {
                                attributes |= CustomUninstallKeyAttributes.Delete;
                            }
                            if (this.Core.GetAttributeValue(sourceLineNumbers, attrib).Equals("write", StringComparison.OrdinalIgnoreCase))
                            {
                                attributes |= CustomUninstallKeyAttributes.Write;
                            }
                            break;

                        default:
                            this.Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    this.Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }

            if (string.IsNullOrEmpty(name))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Name"));
            }

            if (string.IsNullOrEmpty(data))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Data"));
            }

            if (string.IsNullOrEmpty(datatype))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "DataType"));
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
                    if (child.NamespaceURI == this.schema.TargetNamespace)
                    {
                        this.Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        this.Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_Immediate");
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_deferred");
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "CustomUninstallKey_rollback");

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
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == this.schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "destproperty":
                            DestProperty = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "filepath":
                            FilePath = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "section":
                            Section = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "key":
                            Key = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "ignoreerrors":
                            IgnoreErrors = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;

                        default:
                            this.Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    this.Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }

            if (string.IsNullOrEmpty(DestProperty))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "DestProperty"));
            }

            if (string.IsNullOrEmpty(FilePath))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "FilePath"));
            }

            if (string.IsNullOrEmpty(Key))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Key"));
            }

            if (string.IsNullOrEmpty(Section))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Section"));
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
                    if (child.NamespaceURI == this.schema.TargetNamespace)
                    {
                        this.Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        this.Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ReadIniValues");

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
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == this.schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "root":
                            root = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "key":
                            key = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "name":
                            name = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "area":
                            try
                            {
                                area = (RegistryArea)Enum.Parse(typeof(RegistryArea), this.Core.GetAttributeValue(sourceLineNumbers, attrib));
                            }
                            catch
                            {
                                this.Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, node.Name, "Area", this.Core.GetAttributeValue(sourceLineNumbers, attrib)));
                            }
                            break;

                        default:
                            this.Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    this.Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(key))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Key"));
            }
            if (string.IsNullOrEmpty(root))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Root"));
            }
            if (string.IsNullOrEmpty(name))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Name"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == this.schema.TargetNamespace)
                    {
                        this.Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        this.Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "RemoveRegistryValue_Immediate");

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
                this.Core.UnexpectedElement(node.ParentNode, node);
            }
            property = node.ParentNode.Attributes["Id"].Value;

            foreach (XmlAttribute attrib in node.Attributes)
            {
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == this.schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "filepath":
                            filePath = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "xpath":
                            xpath = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "language":
                            lang = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "namespaces":
                            namespaces = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "match":
                            try
                            {
                                match = (XmlSearchMatch)Enum.Parse(typeof(XmlSearchMatch), this.Core.GetAttributeValue(sourceLineNumbers, attrib));
                            }
                            catch
                            {
                                this.Core.OnMessage(WixErrors.ValueNotSupported(sourceLineNumbers, node.Name, "Match", this.Core.GetAttributeValue(sourceLineNumbers, attrib)));
                            }
                            break;

                        default:
                            this.Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    this.Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(property))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(id))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Id"));
            }
            if (string.IsNullOrEmpty(filePath))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "FilePath"));
            }
            if (string.IsNullOrEmpty(xpath))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "XPath"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == this.schema.TargetNamespace)
                    {
                        this.Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        this.Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "XmlSearch");

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

        [Flags]
        private enum TelemetrySequence
        {
            None = 0,
            OnExecute = 1,
            OnCommit = 2,
            OnRollback = 4
        }

        private void ParseTelemetry(XmlNode node)
        {
            SourceLineNumberCollection sourceLineNumbers = Preprocessor.GetSourceLineNumbers(node);
            string id = null;
            string url = null;
            string data = null;
            TelemetrySequence flags = TelemetrySequence.None;
            string condition = "";

            foreach (XmlAttribute attrib in node.Attributes)
            {
                string tmp;
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == this.schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "url":
                            url = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "data":
                            data = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "onsuccess":
                            tmp = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if( tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= TelemetrySequence.OnCommit;
                            }
                            break;
                        case "onstart":
                            tmp = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if( tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= TelemetrySequence.OnExecute;
                            }
                            break;
                        case "onfailure":
                            tmp = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if( tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                flags |= TelemetrySequence.OnRollback;
                            }
                            break;

                        default:
                            this.Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    this.Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(url))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Url"));
            }
            if (string.IsNullOrEmpty(data))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Data"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == this.schema.TargetNamespace)
                    {
                        this.Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        this.Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "Telemetry");

            if (!Core.EncounteredError)
            {
                // create a row in the ReadIniValues table
                Row row = Core.CreateRow(sourceLineNumbers, "PSW_Telemetry");
                row[0] = id;
                row[1] = url;
                row[2] = data;
                row[3] = (int)flags;
                row[4] = condition;
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
            string condition = "";

            foreach (XmlAttribute attrib in node.Attributes)
            {
                string tmp;
                if (0 == attrib.NamespaceURI.Length || attrib.NamespaceURI == this.schema.TargetNamespace)
                {
                    switch (attrib.LocalName.ToLower())
                    {
                        case "id":
                            id = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "target":
                            target = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "args":
                            args = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "workingdir":
                            workDir = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "verb":
                            verb = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            break;
                        case "wait":
                            tmp = this.Core.GetAttributeValue(sourceLineNumbers, attrib);
                            if (tmp.Equals("yes", StringComparison.OrdinalIgnoreCase))
                            {
                                wait = 1;
                            }
                            break;
                        case "show":
                            show = this.Core.GetAttributeIntegerValue(sourceLineNumbers, attrib, 0, 15);
                            break;

                        default:
                            this.Core.UnexpectedAttribute(sourceLineNumbers, attrib);
                            break;
                    }
                }
                else
                {
                    this.Core.UnsupportedExtensionAttribute(sourceLineNumbers, attrib);
                }
            }

            if (string.IsNullOrEmpty(id))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.ParentNode.Name, "Id"));
            }
            if (string.IsNullOrEmpty(target))
            {
                this.Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, node.Name, "Target"));
            }

            // find unexpected child elements
            foreach (XmlNode child in node.ChildNodes)
            {
                if (XmlNodeType.Element == child.NodeType)
                {
                    if (child.NamespaceURI == this.schema.TargetNamespace)
                    {
                        this.Core.UnexpectedElement(node, child);
                    }
                    else
                    {
                        this.Core.UnsupportedExtensionElement(node, child);
                    }
                }
                else if (XmlNodeType.CDATA == child.NodeType || XmlNodeType.Text == child.NodeType)
                {
                    condition = child.Value.Trim();
                }
            }

            // reference the Win32_CopyFiles custom actions since nothing will happen without these
            this.Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "ShellExecute_Immediate");

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
                row[7] = condition;
            }
        }
    }
}
