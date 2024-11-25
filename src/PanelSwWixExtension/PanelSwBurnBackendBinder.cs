using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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
                        PSW_ContainerExtensionData.SymbolDefinition,
                        PSW_CustomSearch.SymbolDefinition,
                        PSW_FileGlob.SymbolDefinition,
                        PSW_FileGlobPattern.SymbolDefinition,
                    };
                }
                return _intermediateSymbols;
            }
        }

        public override bool TryProcessSymbol(IntermediateSection section, IntermediateSymbol symbol)
        {
            foreach (IntermediateSymbolDefinition definition in SymbolDefinitions)
            {
                if (definition.Name == symbol.Definition.Name)
                {
                    return true;
                }
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
            List<WixBundlePayloadSymbol> allPayloadSymbols = new List<WixBundlePayloadSymbol>();
            List<WixBundlePayloadSymbol> myPayloadSymbols = new List<WixBundlePayloadSymbol>();
            List<WixBundleContainerSymbol> containerSymbols = new List<WixBundleContainerSymbol>();
            List<WixGroupSymbol> groupSymbols = new List<WixGroupSymbol>();
            Dictionary<WixBundleContainerSymbol, long> containerSize = new Dictionary<WixBundleContainerSymbol, long>();

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
                        allPayloadSymbols.Add(p);
                        if (BurnConstants.BurnDefaultAttachedContainerName.Equals(p.ContainerRef))
                        {
                            myPayloadSymbols.Add(p);
                        }
                    }
                    else if (symbol is WixBundleContainerSymbol c)
                    {
                        containerSymbols.Add(c);
                    }
                    else if (symbol is WixGroupSymbol g)
                    {
                        groupSymbols.Add(g);
                    }
                }
            }

            if (containerTemplate == null)
            {
                return;
            }

            // Calculate exe size: Sum of payloads in attached containers
            long exeSize = 0;
            foreach (WixBundleContainerSymbol containerSymbol1 in containerSymbols)
            {
                if (containerSymbol1.Type.Equals(ContainerType.Attached) && !BurnConstants.BurnDefaultAttachedContainerName.Equals(containerSymbol1.Id.Id))
                {
                    IEnumerable<WixGroupSymbol> containerPayloadGroups = groupSymbols.Where(g => g.ParentType.Equals(ComplexReferenceParentType.Container) && g.ParentId.Equals(containerSymbol1.Id.Id) && g.ChildType.Equals(ComplexReferenceChildType.Payload));
                    foreach (WixGroupSymbol groupSymbol in containerPayloadGroups)
                    {
                        WixBundlePayloadSymbol payload = allPayloadSymbols.First(p => p.Id.Id.Equals(groupSymbol.ChildId));
                        if (!payload.FileSize.HasValue && File.Exists(payload.SourceFile?.Path))
                        {
                            FileInfo fileInfo = new FileInfo(payload.SourceFile.Path);
                            exeSize += fileInfo.Length;
                        }
                        else
                        {
                            exeSize += payload.FileSize ?? 0;
                        }
                    }
                }
            }

            // Order payloads by package and layout-groups
            myPayloadSymbols.Sort((p1, p2) => SortPayloads(p1, p2, groupSymbols));

            defaultContainer.Type = containerTemplate.DefaultType;
#if EnableZipContainer
            if (containerTemplate.Compression != PSW_ContainerTemplate.ContainerCompressionType.Cab)
            {
                defaultContainer.BootstrapperExtensionRef = PanelSwWixExtension.CONTAINER_EXTENSION_ID;
                BackendHelper.AddBootstrapperExtensionData(PanelSwWixExtension.CONTAINER_EXTENSION_ID, new PSW_ContainerExtensionData(defaultContainer.SourceLineNumbers)
                {
                    Compression = containerTemplate.Compression,
                    ContainerId = defaultContainer.Id.Id
                });
            }
#endif
            // For too-large attached container, grab payloads for the default container
            if ((defaultContainer.Type == ContainerType.Attached) && (containerTemplate.MaximumUncompressedContainerSize > containerTemplate.MaximumUncompressedExeSize))
            {
                for (int i = myPayloadSymbols.Count - 1; i >= 0; --i)
                {
                    WixBundlePayloadSymbol payload = myPayloadSymbols[i];
                    if (!payload.FileSize.HasValue || (containerSize[defaultContainer] + payload.FileSize.Value) < containerTemplate.MaximumUncompressedExeSize)
                    {
                        containerSize[defaultContainer] += payload.FileSize ?? 0;
                        payload.ContainerRef = defaultContainer.Id.Id;
                        myPayloadSymbols.RemoveAt(i);
                    }
                }
            }

            WixBundleContainerSymbol prevContainer = null;
            WixBundlePayloadSymbol prevPayload = null;
            foreach (WixBundlePayloadSymbol payload in myPayloadSymbols)
            {
                WixBundleContainerSymbol container = null;

                // Prefer the previous container if belongs to the same package/layout
                if ((prevContainer != null) && (prevPayload != null) && (SortPayloads(payload, prevPayload, groupSymbols) == 0))
                {
                    long maxContainerSize = ((prevContainer == defaultContainer) && (defaultContainer.Type == ContainerType.Attached) && (containerTemplate.MaximumUncompressedContainerSize > containerTemplate.MaximumUncompressedExeSize))
                        ? containerTemplate.MaximumUncompressedExeSize : containerTemplate.MaximumUncompressedContainerSize;

                    // Previous container has enough capacity ?
                    if (!payload.FileSize.HasValue || ((containerSize[prevContainer] + payload.FileSize.Value) < maxContainerSize))
                    {
                        container = prevContainer;
                    }
                }

                // Find the first container with sufficient size
                if (container == null)
                {
                    foreach (WixBundleContainerSymbol containerSymbol in containerSize.Keys)
                    {
                        // Skip the default container if it is attached and would oversize the exe size
                        if ((containerSymbol == defaultContainer) && (containerSymbol.Type == ContainerType.Attached) && (containerTemplate.MaximumUncompressedExeSize < containerTemplate.MaximumUncompressedContainerSize))
                        {
                            continue;
                        }

                        if (!payload.FileSize.HasValue || (containerSize[containerSymbol] + payload.FileSize.Value) < containerTemplate.MaximumUncompressedContainerSize)
                        {
                            container = containerSymbol;
                            break;
                        }
                    }
                }

                // Need a new container?
                if (container == null)
                {
                    container = section.AddSymbol(new WixBundleContainerSymbol(containerTemplate.SourceLineNumbers, new Identifier(AccessModifier.Global, containerSize.Count)));
                    container.Type = containerTemplate.DefaultType;
#if EnableZipContainer
                    container.BootstrapperExtensionRef = defaultContainer.BootstrapperExtensionRef;
                    if (containerTemplate.Compression != PSW_ContainerTemplate.ContainerCompressionType.Cab)
                    {
                        BackendHelper.AddBootstrapperExtensionData(PanelSwWixExtension.CONTAINER_EXTENSION_ID, new PSW_ContainerExtensionData(container.SourceLineNumbers)
                        {
                            Compression = containerTemplate.Compression,
                            ContainerId = container.Id.Id
                        });
                    }
#endif

                    containerSize[container] = 0;

                    if (payload.FileSize.HasValue && (payload.FileSize.Value > containerTemplate.MaximumUncompressedContainerSize))
                    {
                        Messaging.Write(PanelSwWixErrorMessages.PayloadExceedsSize(payload.SourceLineNumbers, payload.Id.Id, containerTemplate.MaximumUncompressedContainerSize));
                    }
                }

                containerSize[container] += payload.FileSize ?? 0;
                payload.ContainerRef = container.Id.Id;
                prevContainer = container;
                prevPayload = payload;
            }

            // Assign attached/detached and container names
            int detachedCount = 0; // Only detached containers have meaningful names
            foreach (WixBundleContainerSymbol containerSymbol in containerSize.Keys)
            {
                if ((containerTemplate.DefaultType == ContainerType.Attached) && ((containerSize[containerSymbol] + exeSize) <= containerTemplate.MaximumUncompressedExeSize))
                {
                    containerSymbol.Name = $"ctn{Guid.NewGuid().ToString("N")}.dat";
                    containerSymbol.Type = ContainerType.Attached;
                    exeSize += containerSize[containerSymbol];
                }
                else
                {
                    containerSymbol.Name = containerTemplate.CabinetTemplate.Contains("{0}") ? string.Format(containerTemplate.CabinetTemplate, detachedCount++) : containerTemplate.CabinetTemplate;
                    containerSymbol.Type = ContainerType.Detached;
                }
            }
        }

        // Best effort to group payloads by package and payload group (layout)
        private int SortPayloads(WixBundlePayloadSymbol p1, WixBundlePayloadSymbol p2, IEnumerable<WixGroupSymbol> groups)
        {
            WixGroupSymbol p1Group = groups.FirstOrDefault(g => g.ParentType.Equals(ComplexReferenceParentType.Package) && g.ChildType.Equals(ComplexReferenceChildType.Payload) && g.ChildId.Equals(p1.Id.Id));
            WixGroupSymbol p2Group = groups.FirstOrDefault(g => g.ParentType.Equals(ComplexReferenceParentType.Package) && g.ChildType.Equals(ComplexReferenceChildType.Payload) && g.ChildId.Equals(p2.Id.Id));
            if ((p1Group != null) && (p2Group != null))
            {
                return p1Group.ParentId.CompareTo(p2Group.ParentId);
            }
            if ((p1Group != null) || (p2Group != null)) // If one is a package and the other isn't, we place package first
            {
                return (p1Group == null) ? 1 : -1;
            }

            p1Group = groups.FirstOrDefault(g => g.ParentType.Equals(ComplexReferenceParentType.Layout) && g.ChildType.Equals(ComplexReferenceChildType.Payload) && g.ChildId.Equals(p1.Id.Id));
            p2Group = groups.FirstOrDefault(g => g.ParentType.Equals(ComplexReferenceParentType.Layout) && g.ChildType.Equals(ComplexReferenceChildType.Payload) && g.ChildId.Equals(p2.Id.Id));
            if ((p1Group != null) && (p2Group != null))
            {
                return p1Group.ParentId.CompareTo(p2Group.ParentId);
            }
            if ((p1Group != null) || (p2Group != null)) // If one is a layout and the other isn't, we place layout last
            {
                return (p1Group == null) ? -1 : 1;
            }

            return 0;
        }
    }
}
