//===========================================================================
// (c) 2018 Hyland Software, Inc. and its affiliates. All rights reserved.
//===========================================================================

using System;
using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Hyland.DocumentFilters
{
    public abstract class ReadOnlyList<T>
#if NETSTANDARD
        : IReadOnlyList<T>
#else
        : IList<T>
#endif
    {
        protected abstract int GetCount();
        protected abstract T GetItem(int index);
        public T this[int index] { get { return GetItem(index); } set { throw new NotImplementedException(); } }

        public int Count => GetCount();

        public bool IsReadOnly => true;

        public bool Contains(T item)
        {
            return IndexOf(item) >= 0;
        }

        public void CopyTo(T[] array, int arrayIndex)
        {
            for (int i = 0; i < Count; ++i)
                array[arrayIndex + i] = this[i];
        }

        public int IndexOf(T item)
        {
            for (int i = 0; i < Count; ++i)
            {
                if (item.Equals(this[i]))
                    return i;
            }
            return -1;
        }
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

        public IEnumerator<T> GetEnumerator()
        {
            return new Enumerator(this);
        }
        IEnumerator IEnumerable.GetEnumerator()
        {
            return new Enumerator(this);
        }
    }
}
