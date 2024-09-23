using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Xml;
using WixToolset.Data;
using WixToolset.Data.Burn;
using WixToolset.Data.Symbols;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwBurnContainer : BaseBurnContainerExtension
    {
        public override IReadOnlyCollection<string> ContainerExtensionIds => new[] { PanelSwWixExtension.CONTAINER_EXTENSION_ID };

        public override void CreateContainer(WixBundleContainerSymbol container, IEnumerable<WixBundlePayloadSymbol> containerPayloads, WixToolset.Data.CompressionLevel? level, out string sha512, out long size)
        {
            sha512 = null;
            size = 0;
            PSW_ContainerTemplate containerTemplate = null;
            foreach (IntermediateSection section in Context.IntermediateRepresentation.Sections)
            {
                containerTemplate = section.Symbols.FirstOrDefault(s => s is PSW_ContainerTemplate) as PSW_ContainerTemplate;
                if (containerTemplate != null)
                {
                    break;
                }
            }
            if (containerTemplate == null)
            {
                Messaging.Write(PanelSwWixErrorMessages.MissingContainerTemplate(container.SourceLineNumbers, container.Id.Id));
                return;
            }

            try
            {
                if (File.Exists(container.WorkingPath))
                {
                    File.Delete(container.WorkingPath);
                }

                switch (containerTemplate.Compression)
                {
                    case PSW_ContainerTemplate.ContainerCompressionType.Zip:
                        CreateContainerZip(container, containerPayloads);
                        break;
                    case PSW_ContainerTemplate.ContainerCompressionType.SevenZip:
                        CreateContainerLzma(container, containerPayloads, level);
                        break;
                }
                CalculateHashAndSize(container.WorkingPath, out sha512, out size);
            }
            catch (Exception ex)
            {
                Messaging.Write(PanelSwWixErrorMessages.ContainerError(container.SourceLineNumbers, container.Id.Id, ex.Message));
                return;
            }
        }

        private void CreateContainerZip(WixBundleContainerSymbol container, IEnumerable<WixBundlePayloadSymbol> containerPayloads)
        {
            using (ZipArchive zipFile = ZipFile.Open(container.WorkingPath, ZipArchiveMode.Create))
            {
                foreach (WixBundlePayloadSymbol payload in containerPayloads)
                {
                    string entryName = payload.EmbeddedId;
                    FileInfo fileInfo = new FileInfo(payload.SourceFile.Path);

                    ZipArchiveEntry entry = zipFile.CreateEntry(entryName);
                    entry.LastWriteTime = fileInfo.LastWriteTime;
                    using (Stream ws = entry.Open())
                    {
                        using (FileStream rs = File.Open(fileInfo.FullName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete))
                        {
                            rs.CopyTo(ws);
                        }
                    }
                }
            }
        }

        private void CreateContainerLzma(WixBundleContainerSymbol container, IEnumerable<WixBundlePayloadSymbol> containerPayloads, WixToolset.Data.CompressionLevel? level)
        {
            string xmlFile = null;
            XmlDocument xmlDocument = null;
            List<WixBundlePayloadSymbol> filteredPayloads = new List<WixBundlePayloadSymbol>(containerPayloads);

            IEnumerable<IGrouping<string, WixBundlePayloadSymbol>> payloadsBySourcePath = containerPayloads.GroupBy(pld => Path.GetFullPath(pld.SourceFile.Path));
            foreach (var pldGrp in payloadsBySourcePath)
            {
                if (pldGrp.Count() > 1)
                {
                    if (string.IsNullOrEmpty(xmlFile))
                    {
                        xmlFile = Path.GetTempFileName();
                        xmlDocument = new XmlDocument();
                        XmlElement root = xmlDocument.CreateElement("Root");
                        xmlDocument.AppendChild(root);

                        XmlDeclaration xmlDeclaration = xmlDocument.CreateXmlDeclaration("1.0", "UTF-8", null);
                        xmlDocument.InsertBefore(xmlDeclaration, root);
                    }

                    WixBundlePayloadSymbol srcPld = pldGrp.ElementAt(0);
                    for (int i = pldGrp.Count() - 1; i > 0; --i)
                    {
                        WixBundlePayloadSymbol pld = pldGrp.ElementAt(i);
                        filteredPayloads.Remove(pld);

                        XmlElement mapping = xmlDocument.CreateElement("Mapping");
                        mapping.SetAttribute("Source", srcPld.EmbeddedId);
                        mapping.SetAttribute("Target", pld.EmbeddedId);
                        xmlDocument.DocumentElement.AppendChild(mapping);
                    }
                    xmlDocument.Save(xmlFile);
                }
            }

            List<SevenZap.SevenZap.FileEntry> entries = new List<SevenZap.SevenZap.FileEntry>(filteredPayloads.Select(
                p => new SevenZap.SevenZap.FileEntry()
                {
                    EntryName = p.EmbeddedId,
                    FullPath = p.SourceFile.Path
                }));

            if (!string.IsNullOrEmpty(xmlFile))
            {
                entries.Add(new SevenZap.SevenZap.FileEntry { EntryName = PanelSwWixExtension.CONTAINER_EXTENSION_ID, FullPath = xmlFile });
            }

            // Compression level
            SevenZap.SevenZap.CompressionLevel level7z = SevenZap.SevenZap.CompressionLevel.X5_Medium;
            switch (level)
            {
                case WixToolset.Data.CompressionLevel.None:
                case WixToolset.Data.CompressionLevel.Low:
                    level7z = SevenZap.SevenZap.CompressionLevel.X1_Fastest;
                    break;
                case WixToolset.Data.CompressionLevel.Medium:
                    level7z = SevenZap.SevenZap.CompressionLevel.X5_Medium;
                    break;
                case WixToolset.Data.CompressionLevel.High:
                    level7z = SevenZap.SevenZap.CompressionLevel.X9_Smallest;
                    break;
            }

            SevenZap.SevenZap.UpdateArchive(container.WorkingPath, entries, Context.CancellationToken, level7z);

            if (!string.IsNullOrEmpty(xmlFile))
            {
                File.Delete(xmlFile);
            }
        }

        public override void ExtractContainer(string containerPath, string outputFolder, string containerId, XmlElement extensionDataNode)
        {
            XmlNamespaceManager nmspc = new XmlNamespaceManager(extensionDataNode.OwnerDocument.NameTable);
            nmspc.AddNamespace("ext", BurnConstants.BootstrapperExtensionDataNamespace);

            XmlNode compressionAttr = extensionDataNode.SelectSingleNode($"//ext:PSW_ContainerExtensionData[@ContainerId='{containerId}']/@Compression", nmspc);
            if ((compressionAttr == null) || !Enum.TryParse(compressionAttr.Value, out PSW_ContainerTemplate.ContainerCompressionType compression))
            {
                throw new ArgumentException("Can't extract container since manifest does not contain required extension data");
            }

            switch (compression)
            {
                case PSW_ContainerTemplate.ContainerCompressionType.Zip:
                    ExtractContainerZip(containerPath, outputFolder);
                    break;
                case PSW_ContainerTemplate.ContainerCompressionType.SevenZip:
                    ExtractContainerLzma(containerPath, outputFolder);
                    break;
                default:
                    throw new ArgumentException($"Can't extract container with {compression} compression");
            }
        }

        private void ExtractContainerZip(string containerPath, string outputFolder)
        {
            ZipFile.ExtractToDirectory(containerPath, outputFolder);
        }

        private void ExtractContainerLzma(string containerPath, string outputFolder)
        {
            SevenZap.SevenZap.Extract(containerPath, outputFolder);

            string xmlFile = Path.Combine(outputFolder, PanelSwWixExtension.CONTAINER_EXTENSION_ID);
            if (File.Exists(xmlFile))
            {
                XmlDocument xmlDocument = new XmlDocument();
                xmlDocument.Load(xmlFile);
                XmlNodeList mappings = xmlDocument.SelectNodes("/Root/Mapping");
                foreach (XmlNode mappingsNode in mappings)
                {
                    XmlElement mapping = (XmlElement)mappingsNode;
                    string srcFile = Path.Combine(outputFolder, mapping.GetAttribute("Source"));
                    string dstFile = Path.Combine(outputFolder, mapping.GetAttribute("Target"));
                    if (File.Exists(srcFile))
                    {
                        File.Copy(srcFile, dstFile, true);
                    }
                }
                File.Delete(xmlFile);
            }
        }
    }
}
