//===========================================================================
// (c) 2019 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{
    /// <summary>
    /// Abstract class that represents a read-only list.
    /// </summary>
    /// <typeparam name="T">Type of child</typeparam>
    public abstract class ReadOnlyList<T>
#if NETSTANDARD
        : IReadOnlyList<T>
#else
        : IList<T>
#endif
    {
        /// <summary>
        /// Returns the number of items in the list.
        /// </summary>
        protected abstract int GetCount();

        /// <summary>
        /// Returns the item at the specified index.
        /// </summary>
        protected abstract T GetItem(int index);

        /// <summary>
        /// Returns the item at the specified index.
        /// </summary>
        public T this[int index] { get { return GetItem(index); } set { throw new NotImplementedException(); } }

        /// <summary>
        /// Returns the number of items in the list.
        /// </summary>
        public int Count => GetCount();

        /// <summary>
        /// Indicates whether the list is read-only.
        /// </summary>
        public bool IsReadOnly => true;

        /// <summary>
        /// Indicates whether the list contains the specified item.
        /// </summary>
        public bool Contains(T item)
        {
            return IndexOf(item) >= 0;
        }

        /// <summary>
        /// Copies the items in the list to the specified array.
        /// </summary>
        public void CopyTo(T[] array, int arrayIndex)
        {
            for (int i = 0; i < Count; ++i)
                array[arrayIndex + i] = this[i];
        }

        /// <summary>
        /// Returns the index of the specified item.
        /// </summary>
        public int IndexOf(T item)
        {
            for (int i = 0; i < Count; ++i)
            {
                if (item.Equals(this[i]))
                    return i;
            }
            return -1;
        }

#pragma warning disable 1591
#if !NETSTANDARD

        public void Add(T item)
        {
            throw new NotImplementedException();
        }

        public void Clear()
        {
            throw new NotImplementedException();
        }


        public void Insert(int index, T item)
        {
            throw new NotImplementedException();
        }

        public bool Remove(T item)
        {
            throw new NotImplementedException();
        }

        public void RemoveAt(int index)
        {
            throw new NotImplementedException();
        }
#endif
#pragma warning restore 1591

        internal class Enumerator : IEnumerator<T>
        {
            private int _index;
            private ReadOnlyList<T> _parent;

            public T Current => GetCurrent();

            object IEnumerator.Current => GetCurrent();

            public void Dispose()
            {
            }

            public bool MoveNext()
            {
                return (++_index < _parent.Count);
            }

            public void Reset()
            {
                _index = -1;
            }
            private T GetCurrent()
            {
                return _parent[_index];
            }
            internal Enumerator(ReadOnlyList<T> parent)
            {
                _parent = parent;
                _index = -1;
            }
        }

        /// <summary>
        /// Returns an enumerator for the list.
        /// </summary>
        public IEnumerator<T> GetEnumerator()
        {
            return new Enumerator(this);
        }

        /// <summary>
        /// Returns an enumerator for the list.
        /// </summary>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return new Enumerator(this);
        }
    }
}
