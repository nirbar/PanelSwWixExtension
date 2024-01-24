using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.ComponentModel;
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

        public override void CreateContainer(WixBundleContainerSymbol container, IEnumerable<WixBundlePayloadSymbol> containerPayloads, out string sha512, out long size)
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
                        CreateContainerLzma(container, containerPayloads);
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

        private void CreateContainerLzma(WixBundleContainerSymbol container, IEnumerable<WixBundlePayloadSymbol> containerPayloads)
        {
            List<SevenZap.SevenZap.FileEntry> entries = new List<SevenZap.SevenZap.FileEntry>(containerPayloads.Select(
                p => new SevenZap.SevenZap.FileEntry()
                {
                    EntryName = p.EmbeddedId,
                    FullPath = p.SourceFile.Path
                }));
            SevenZap.SevenZap.UpdateArchive(container.WorkingPath, entries);
        }

        public override void ExtractContainer(string containerPath, string outputFolder, string containerId, XmlElement extensionDataNode)
        {
            XmlNamespaceManager nmspc = new XmlNamespaceManager(extensionDataNode.OwnerDocument.NameTable);
            nmspc.AddNamespace("ext", BurnConstants.BundleExtensionDataNamespace);

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
        }
    }
}
