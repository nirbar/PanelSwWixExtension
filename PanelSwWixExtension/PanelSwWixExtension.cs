using System;
using System.Collections.Generic;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public sealed class PanelSwWixExtension : BaseExtensionFactory
    {
        protected override IReadOnlyCollection<Type> ExtensionTypes => new Type[]
        {
            typeof(PanelSwWixPreprocessor),
            typeof(PanelSwWiBackendBinder),
            typeof(PanelSwWixCompiler),
            typeof(PanelSwWixExtData),
        };
    }
}
