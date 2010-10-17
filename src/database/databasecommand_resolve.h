#ifndef DATABASECOMMAND_RESOLVE_H
#define DATABASECOMMAND_RESOLVE_H
#include "databasecommand.h"
#include "databaseimpl.h"
#include "tomahawk/result.h"
#include <QVariant>

class DatabaseCommand_Resolve : public DatabaseCommand
{
Q_OBJECT
public:
    //explicit DatabaseCommand_Resolve(QObject *parent = 0);
    explicit DatabaseCommand_Resolve( QVariant v, bool searchlocal );

    virtual QString commandname() const { return "dbresolve"; }
    virtual bool doesMutates() const { return false; }

    virtual void exec(DatabaseImpl *lib);


signals:

    void results( Tomahawk::QID qid, QList<Tomahawk::result_ptr> results );

public slots:

private:
    QVariant m_v;
    bool m_searchlocal;

    float how_similar( const QVariantMap& q, const QVariantMap& r );

    static int levenshtein(const QString& source, const QString& target)
        {
          // Step 1
          const int n = source.length();
          const int m = target.length();
          if (n == 0) {
            return m;
          }
          if (m == 0) {
            return n;
          }
          // Good form to declare a TYPEDEF
          typedef QVector< QVector<int> > Tmatrix;
          Tmatrix matrix;
          matrix.resize( n+1 );

          // Size the vectors in the 2.nd dimension. Unfortunately C++ doesn't
          // allow for allocation on declaration of 2.nd dimension of vec of vec
          for (int i = 0; i <= n; i++) {
            QVector<int> tmp;
            tmp.resize( m+1 );
            matrix.insert( i, tmp );
          }
          // Step 2
          for (int i = 0; i <= n; i++) {
            matrix[i][0]=i;
          }
          for (int j = 0; j <= m; j++) {
            matrix[0][j]=j;
          }
          // Step 3
          for (int i = 1; i <= n; i++) {
            const QChar s_i = source[i-1];
            // Step 4
            for (int j = 1; j <= m; j++) {
              const QChar t_j = target[j-1];
              // Step 5
              int cost;
              if (s_i == t_j) {
                cost = 0;
              }
              else {
                cost = 1;
              }
              // Step 6
              const int above = matrix[i-1][j];
              const int left = matrix[i][j-1];
              const int diag = matrix[i-1][j-1];
              //int cell = min( above + 1, min(left + 1, diag + cost));
              int cell = (((left+1)>(diag+cost))?diag+cost:left+1);
              if(above+1 < cell) cell = above+1;
              // Step 6A: Cover transposition, in addition to deletion,
              // insertion and substitution. This step is taken from:
              // Berghel, Hal ; Roach, David : "An Extension of Ukkonen's
              // Enhanced Dynamic Programming ASM Algorithm"
              // (http://www.acm.org/~hlb/publications/asm/asm.html)
              if (i>2 && j>2) {
                int trans=matrix[i-2][j-2]+1;
                if (source[i-2]!=t_j) trans++;
                if (s_i!=target[j-2]) trans++;
                if (cell>trans) cell=trans;
              }
              matrix[i][j]=cell;
            }
          }
          // Step 7
          return matrix[n][m];
        };
};

#endif // DATABASECOMMAND_RESOLVE_H
