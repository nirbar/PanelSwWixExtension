using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Xml.Linq;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Data.WindowsInstaller;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwWixDecompiler : BaseWindowsInstallerDecompilerExtension
    {
        public override IReadOnlyCollection<TableDefinition> TableDefinitions => PanelSwWixExtension.TableDefinitions;

        public override bool TryDecompileTable(Table table)
        {
            switch (table.Name)
            {
                case "PSW_AccountSidSearch":
                    this.DecompileAccountSidSearch(table);
                    break;
                default:
                    return false;
            }

            return true;
        }

        private void DecompileAccountSidSearch(Table table)
        {
            foreach (var row in table.Rows)
            {
                string propName = row.FieldAsString(1);
                XElement xProperty = null;
                if (!this.DecompilerHelper.TryGetIndexedElement("Property", propName, out xProperty))
                {
                    xProperty = this.DecompilerHelper.AddElementToRoot("Property");
                    xProperty.SetAttributeValue("Id", propName);
                }

                XElement xAccountSidSearch = new XElement(PanelSwWixExtension.Namespace + "AccountSidSearch");
                string systemName = row.FieldAsString(2);
                if (!string.IsNullOrEmpty(systemName))
                {
                    xAccountSidSearch.SetAttributeValue("SystemName", systemName);
                }
                string accountName = row.FieldAsString(3);
                if (!string.IsNullOrEmpty(accountName))
                {
                    xAccountSidSearch.SetAttributeValue("AccountName", accountName);
                }
                string condition = row.FieldAsString(4);
                if (!string.IsNullOrEmpty(condition))
                {
                    xAccountSidSearch.SetAttributeValue("Condition", condition);
                }

                xProperty.Add(xAccountSidSearch);
            }
        }
    }
}
