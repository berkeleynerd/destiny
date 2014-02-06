    #include "Treap.h"
	bool Less(const PyObject *k1,const PyObject *k2);



    /**
     * Implements an unbalanced binary search tree.
     * Note that all "matching" is based on the compares method.
     * @author Mark Allen Weiss
     */
        /**
         * Construct the treap.
         */
        Treap::Treap(  PyObject* notFound ) :
          ITEM_NOT_FOUND( notFound )
        {
            nullNode = new TreapNode;
            nullNode->left = nullNode->right = nullNode;
            nullNode->priority = INT_MAX;
            root = nullNode;
        }

        /**
         * Copy constructor.
         */
        Treap::Treap( const Treap & rhs )
          : ITEM_NOT_FOUND( rhs.ITEM_NOT_FOUND )
        {
            nullNode = new TreapNode;
            nullNode->left = nullNode->right = nullNode;
            nullNode->priority = INT_MAX;
            root = nullNode;
            *this = rhs;
        }

        /**
         * Destructor for the tree.
         */
        Treap::~Treap( )
        {
            makeEmpty( );
			delete nullNode;
        }

        /**
         * Insert x into the tree; duplicates are ignored.
         */
        void Treap::insert(  PyObject* & x )
        {
            insert( x, root );
        }

        /**
         * Remove x from the tree. Nothing is done if x is not found.
         */
        bool Treap::remove(  PyObject* & x )
        {
            return remove( x, root );
        }

        /**
         * Find the smallest item in the tree.
         * Return smallest item or ITEM_NOT_FOUND if empty.
         */
        PyObject* & Treap::findMin( )
        {
            if( isEmpty( ) )
                return ITEM_NOT_FOUND;

            TreapNode *ptr = root;
            while( ptr->left != nullNode )
                ptr = ptr->left;

            return ptr->element;
        }

        /**
         * Find the largest item in the tree.
         * Return the largest item of ITEM_NOT_FOUND if empty.
         */
        PyObject* & Treap::findMax( )
        {
            if( isEmpty( ) )
                return ITEM_NOT_FOUND;

            TreapNode *ptr = root;
            while( ptr->right != nullNode )
                ptr = ptr->right;

            return ptr->element;
        }

        /**
         * Find item x in the tree.
         * Return the matching item or ITEM_NOT_FOUND if not found.
         */
        PyObject* & Treap::
        find(PyObject* & x )
        {
            TreapNode *current = root;
            nullNode->element = x;

            for( ; ; )
            {
                if( Less(x, current->element) )
                    current = current->left;
                else if( Less(current->element, x) )
                    current = current->right;
                else if( current != nullNode )
                    return current->element;
                else
                    return ITEM_NOT_FOUND;
            }
        }

        /**
         * Make the tree logically empty.
         */
        void Treap::makeEmpty( )
        {
            makeEmpty( root );
        }

        /**
         * Test if the tree is logically empty.
         * Return true if empty, false otherwise.
         */
        bool Treap::isEmpty( ) const
        {
            return root == nullNode;
        }

        /**
         * Deep copy.
         */
        const Treap &
        Treap::operator=( const Treap & rhs )
        {
            if( this != &rhs )
            {
                makeEmpty( );
                root = clone( rhs.root );
            }

            return *this;
        }

        /**
         * Internal method to insert into a subtree.
         * x is the item to insert.
         * t is the node that roots the tree.
         * Set the new root. random
         */
        void Treap::
        insert(  PyObject* & x, TreapNode * & t )
        {
            if( t == nullNode )
                t = new TreapNode( x, nullNode, nullNode,
                                               randomNums.randomInt( ) );
            else if( Less(x, t->element) )
            {
                insert( x, t->left );
                if( t->left->priority < t->priority )
                    rotateWithLeftChild( t );
            }
            else if( Less(t->element, x) )
            {
                insert( x, t->right );
                if( t->right->priority < t->priority )
                    rotateWithRightChild( t );
            }
            // else duplicate; do nothing
        }

        /**
         * Internal method to remove from a subtree.
         * x is the item to remove.
         * t is the node that roots the tree.
         * Set the new root.
         */
        bool Treap::remove(  PyObject* & x,
                                        TreapNode * & t )
        {
            if( t != nullNode )
            {
                if( Less(x, t->element) )
                    return remove( x, t->left );
                else if( Less(t->element, x) )
                    return remove( x, t->right );
                else
                {
                        // Match found
                    if( t->left->priority < t->right->priority )
                        rotateWithLeftChild( t );
                    else
                        rotateWithRightChild( t );

                    if( t != nullNode )      // Continue on down
                        return remove( x, t );
                    else
                    {
                        delete t->left;
                        t->left = nullNode;  // At a leaf
						return true;
                    }
                }
            }
			else
			{
				return false;
			}

			return false;
        }

        /**
         * Internal method to make subtree empty.
         */
        void Treap::makeEmpty( TreapNode * & t )
        {
            if( t != nullNode )
            {
                makeEmpty( t->left );
                makeEmpty( t->right );
                delete t;
            }
            t = nullNode;
        }


        /**
         * Internal method to clone subtree.
         */
        TreapNode *
        Treap::clone( TreapNode * t )
        {
            if( t == t->left )  // Cannot test against nullNode!!!
                return nullNode;
            else
                return new TreapNode( t->element, clone( t->left ),
                                               clone( t->right ), t->priority );
        }

        /**
         * Rotate binary tree node with left child.
         */
        void Treap::rotateWithLeftChild( TreapNode * & k2 ) const
        {
            TreapNode *k1 = k2->left;
            k2->left = k1->right;
            k1->right = k2;
            k2 = k1;
        }

        /**
         * Rotate binary tree node with right child.
         */
        void Treap::rotateWithRightChild( TreapNode * & k1 ) const
        {
            TreapNode *k2 = k1->right;
            k1->right = k2->left;
            k2->left = k1;
            k1 = k2;
        }
