using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
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

            try
            {
                if (File.Exists(container.WorkingPath))
                {
                    File.Delete(container.WorkingPath);
                }
                using (ZipArchive zipFile = ZipFile.Open(container.WorkingPath, ZipArchiveMode.Create))
                {
                    foreach (WixBundlePayloadSymbol payload in containerPayloads)
                    {
                        string entryName = payload.EmbeddedId;
                        FileInfo fileInfo = new FileInfo(payload.SourceFile.Path);

                        // Skip adding same file if unmodified
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
            catch (Exception ex)
            {
                Messaging.Write(PanelSwWixErrorMessages.ContainerError(container.SourceLineNumbers, container.Id.Id, ex.Message));
                return;
            }

            CalculateHashAndSize(container.WorkingPath, out sha512, out size);
        }
    }
}
