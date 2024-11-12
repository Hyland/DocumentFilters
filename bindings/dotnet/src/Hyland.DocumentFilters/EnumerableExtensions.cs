// Derived from .NET Reference Source 
// https://github.com/microsoft/referencesource/blob/master/System.Core/System/Linq/Enumerable.cs
// Licensed under the MIT license

using System;
using System.Collections.Generic;

namespace Hyland.DocumentFilters
{
    internal static class EnumerableExtensions
    {
        struct Buffer<TElement>
        {
            internal TElement[] items;
            internal int count;

            internal Buffer(IEnumerable<TElement> source)
            {
                TElement[] items = null;
                int count = 0;
                ICollection<TElement> collection = source as ICollection<TElement>;
                if (collection != null)
                {
                    count = collection.Count;
                    if (count > 0)
                    {
                        items = new TElement[count];
                        collection.CopyTo(items, 0);
                    }
                }
                else
                {
                    foreach (TElement item in source)
                    {
                        if (items == null)
                        {
                            items = new TElement[4];
                        }
                        else if (items.Length == count)
                        {
                            TElement[] newItems = new TElement[checked(count * 2)];
                            Array.Copy(items, 0, newItems, 0, count);
                            items = newItems;
                        }
                        items[count] = item;
                        count++;
                    }
                }
                this.items = items;
                this.count = count;
            }

            internal TElement[] ToArray()
            {
                if (count == 0) return new TElement[0];
                if (items.Length == count) return items;
                TElement[] result = new TElement[count];
                Array.Copy(items, 0, result, 0, count);
                return result;
            }
        }

        public static IEnumerable<TSource> ReverseIterator<TSource>(IEnumerable<TSource> source)
        {
            Buffer<TSource> buffer = new Buffer<TSource>(source);
            for (int i = buffer.count - 1; i >= 0; i--) yield return buffer.items[i];
        }
    }
}
