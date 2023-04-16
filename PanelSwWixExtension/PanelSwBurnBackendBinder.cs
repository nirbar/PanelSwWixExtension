using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using WixToolset.Data;
using WixToolset.Data.Burn;
using WixToolset.Data.Symbols;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    internal class PanelSwBurnBackendBinder : BaseBurnBackendBinderExtension
    {
        private List<IntermediateSymbolDefinition> _intermediateSymbols;
        protected override IReadOnlyCollection<IntermediateSymbolDefinition> SymbolDefinitions
        {
            get
            {
                if (_intermediateSymbols == null)
                {
                    _intermediateSymbols = new List<IntermediateSymbolDefinition>
                    {
                        PSW_ContainerTemplate.SymbolDefinition,
                    };
                }
                return _intermediateSymbols;
            }
        }

        public override bool TryProcessSymbol(IntermediateSection section, IntermediateSymbol symbol)
        {
            if (symbol is PSW_ContainerTemplate)
            {
                return true;
            }
            return base.TryProcessSymbol(section, symbol);
        }

        public override void SymbolsFinalized(IntermediateSection section)
        {
            FinalizeAutoContainers(section);
            base.SymbolsFinalized(section);
        }

        private void FinalizeAutoContainers(IntermediateSection section)
        {
            PSW_ContainerTemplate containerTemplate = null;
            WixBundleContainerSymbol defaultContainer = null;
            List<WixBundlePayloadSymbol> payloadSymbols = new List<WixBundlePayloadSymbol>();
            Dictionary<WixBundleContainerSymbol, long> containerSize = new Dictionary<WixBundleContainerSymbol, long>();
            long exeSize = 0;

            foreach (IntermediateSection intermediate in Context.IntermediateRepresentation.Sections)
            {
                foreach (IntermediateSymbol symbol in intermediate.Symbols)
                {
                    if (symbol is PSW_ContainerTemplate containerTemplate1)
                    {
                        containerTemplate = containerTemplate1;
                    }
                    else if ((symbol is WixBundleContainerSymbol container) && container.Id.Id.Equals(BurnConstants.BurnDefaultAttachedContainerName))
                    {
                        defaultContainer = container;
                        containerSize[defaultContainer] = 0;
                    }
                    else if (symbol is WixBundlePayloadSymbol p)
                    {
                        switch (p.ContainerRef)
                        {
                            case BurnConstants.BurnDefaultAttachedContainerName:
                                payloadSymbols.Add(p);
                                break;
                            case BurnConstants.BurnUXContainerName:
                                if (!p.FileSize.HasValue && File.Exists(p.SourceFile?.Path))
                                {
                                    FileInfo fileInfo = new FileInfo(p.SourceFile.Path);
                                    exeSize += fileInfo.Length;
                                }
                                else
                                {
                                    exeSize += p.FileSize ?? 0;
                                }
                                break;
                        }
                    }
                }
            }

            if (containerTemplate == null)
            {
                return;
            }

            // Best effort to group payloads by package
            payloadSymbols.Sort((p1, p2) => p1.ParentPackagePayloadRef?.CompareTo(p2.ParentPackagePayloadRef) ?? 0);

            defaultContainer.Name = string.Format(containerTemplate.CabinetTemplate, 0);
            defaultContainer.Type = containerTemplate.DefaultType;
            foreach (WixBundlePayloadSymbol payload in payloadSymbols)
            {
                WixBundleContainerSymbol container = null;

                foreach (WixBundleContainerSymbol containerSymbol in containerSize.Keys)
                {
                    if (!payload.FileSize.HasValue || (containerSize[containerSymbol] + payload.FileSize.Value) < containerTemplate.MaximumUncompressedContainerSize)
                    {
                        container = containerSymbol;
                        break;
                    }
                }

                // Need a new container?
                if (container == null)
                {
                    container = section.AddSymbol(new WixBundleContainerSymbol(containerTemplate.SourceLineNumbers, new Identifier(AccessModifier.Global, containerSize.Count)));
                    container.Name = string.Format(containerTemplate.CabinetTemplate, containerSize.Count);
                    container.Type = containerTemplate.DefaultType;
                    containerSize[container] = 0;

                    if (payload.FileSize.HasValue && (payload.FileSize.Value > containerTemplate.MaximumUncompressedContainerSize))
                    {
                        Messaging.Write(new Message(payload.SourceLineNumbers, MessageLevel.Warning, 0, "Payload {0} is larger than the maximal size set in ContainerTemplate, {1}", payload.Id.Id, containerTemplate.MaximumUncompressedContainerSize));
                    }
                }

                containerSize[container] += payload.FileSize ?? 0;
                payload.ContainerRef = container.Id.Id;
            }

            // Assign attached/detached
            if (containerTemplate.DefaultType == ContainerType.Attached)
            {
                int detachedCount = 0; // Only detached containers have meaningful names
                foreach (WixBundleContainerSymbol containerSymbol in containerSize.Keys)
                {
                    if ((containerSize[containerSymbol] + exeSize) < containerTemplate.MaximumUncompressedExeSize)
                    {
                        containerSymbol.Name = $"cab{Guid.NewGuid().ToString("N")}.cab";
                        containerSymbol.Type = ContainerType.Attached;
                        exeSize += containerSize[containerSymbol];
                    }
                    else
                    {
                        containerSymbol.Name = string.Format(containerTemplate.CabinetTemplate, detachedCount++);
                        containerSymbol.Type = ContainerType.Detached;
                    }
                }
            }
        }
    }
}
