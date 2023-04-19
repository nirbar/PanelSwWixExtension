using System;
using System.Collections.Generic;
using WixToolset.Extensibility;

namespace PanelSw.Wix.HarvestExtension
{
    public sealed class PanelSwWixHarvestExtensionFactory : BaseExtensionFactory
    {
        protected override IReadOnlyCollection<Type> ExtensionTypes => new Type[]
        {
            typeof(PanelSwWixHarvestExtension),
        };

#if DEBUG
        public override bool TryCreateExtension(Type extensionType, out object extension)
        {
            System.Diagnostics.Debugger.Launch();
            return base.TryCreateExtension(extensionType, out extension);
        }
#endif
    }
}
