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
            Core.EnsureTable(null, "CustomUninstallKey");
            Core.EnsureTable(null, "ReadIniValues");
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
                Row row = Core.CreateRow(sourceLineNumbers, "CustomUninstallKey");
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
                Row row = Core.CreateRow(sourceLineNumbers, "ReadIniValues");
                row[0] = id;
                row[1] = FilePath;
                row[2] = Section;
                row[3] = Key;
                row[4] = DestProperty;
                row[5] = Attributes;
                row[6] = condition;
            }
        }

    }
}
