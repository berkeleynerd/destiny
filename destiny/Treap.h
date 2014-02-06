        #ifndef TREAP_H_
        #define TREAP_H_

        #include <limits.h>
        #include "Random.h"
        //#include "dsexceptions.h"

        // Treap class
        //
        // CONSTRUCTION: with ITEM_NOT_FOUND object used to signal failed finds
        //
        // ******************PUBLIC OPERATIONS*********************
        // void insert( x )       --> Insert x
        // void remove( x )       --> Remove x
        // Comparable find( x )   --> Return item that matches x
        // Comparable findMin( )  --> Return smallest item
        // Comparable findMax( )  --> Return largest item
        // boolean isEmpty( )     --> Return true if empty; else false
        // void makeEmpty( )      --> Remove all items
        // void printTree( )      --> Print tree in sorted order


          // Node and forward declaration because g++ does
          // not understand nested classes.
        class Treap;
        class TreapNode
        {
            PyObject *element;
            TreapNode *left;
            TreapNode *right;
            int        priority;

            TreapNode( ) : element(0), left( 0 ), right( 0 ), priority( INT_MAX ) { }
            TreapNode( PyObject* & theElement, TreapNode *lt, TreapNode *rt, int pr )
              : element( theElement ), left( lt ), right( rt ), priority( pr )
              { }

            friend class Treap;
        };

		class Treap
        {
          public:
            explicit Treap( PyObject* notFound );
            Treap( const Treap & rhs );
            ~Treap( );

            PyObject* & findMin( );
            PyObject* & findMax( );
            PyObject* & find( PyObject* & x );
            bool isEmpty( ) const;
            void printTree( ) const;

            void makeEmpty( );
            void insert( PyObject* & x );
            bool remove( PyObject* & x );

            const Treap & operator=( const Treap & rhs );

          private:
            TreapNode *root;
            PyObject* ITEM_NOT_FOUND;
            TreapNode *nullNode;
            Random randomNums;

              // Recursive routines
            void insert( PyObject* & x, TreapNode * & t );
            bool remove( PyObject* & x, TreapNode * & t );
            void makeEmpty( TreapNode * & t );
            void printTree( TreapNode *t ) const;

              // Rotations
            void rotateWithLeftChild( TreapNode * & t ) const;
            void rotateWithRightChild( TreapNode * & t ) const;
            TreapNode * clone( TreapNode * t );
        };

        #include "Treap.cpp"
        #endif
