#! /bin/sh
CURRDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

directory=""
clang_dir=""
llvm_dir="/home/ali_abyaneh/llvm-project"
while [ "$1" != "" ]; do
   case "$1" in
   "--directory" | "-d")
     shift
     directory=$1
     ;;
    "--llvm-dir")
      shift
      clang_dir=$1
      ;;
     "--help" | "-h")
      echo "Please Note the the passes should be inserted in LLVM lib, and added to the corresponding CMake File"
      echo ""
      echo "Required Commands are:"
      echo ""
      echo "--directory or -d               The directory for files on which analysis should be done"
      echo "--llvm-dir                      The directory of the compiled llvm project"
      exit 1
      ;;
     *)
     echo -n "unknown command"
     ;;
   esac
   shift
done

LOGDIR="${CURRDIR}/${directory}log"
LLVM_PATH="${llvm_dir}/build"
export CLANG="${LLVM_PATH}/bin/clang"
export OPT="${LLVM_PATH}/bin/opt"
HELLO="${LLVM_PATH}/lib/LLVMMyHello.so"
MYHELLO="-myhello"
ITRBBPATH="${LLVM_PATH}/lib/LLVMitrBB.so"
ITRBB="-itrBB"
ITRINSIDEBBPATH="${LLVM_PATH}/lib/LLVMitrInsideBB.so"
ITRINSIDEBB="-itrInsideBB"
COUNTCALLSPATH="${LLVM_PATH}/lib/LLVMcallCounter.so"
COUNTCALLS="-callCounter"
IRFILESUFFIX=".ll"
BCFILESUFFIX=".bc"
LOGSUFFIX=".log"
LOOPPATH="${LLVM_PATH}/lib/LLVMPLoopFinder.so"
LOOP="-PLoopFinder"
DSPATH="${LLVM_PATH}/lib/LLVMDependenceScorer.so"
DS="-DependenceScorer"
INSTRUMENTPATH="${LLVM_PATH}/lib/LLVMMInstrument.so"
INSTRUMENT="-MInstrument"

echo $LOGDIR
cd ${directory}
rm -r $LOGDIR
mkdir $LOGDIR
for f in $(find . -name '*.c'); do
    $CLANG  -O0  -emit-llvm -c ${f} -o "${f%.*}${IRFILESUFFIX}"
    # $OPT -analyze -mem2reg -loop-rotate -indvars -loop-simplify -scalar-evolution < "${f%.*}${IRFILESUFFIX}"
    STRIPPEDNAME=${f#.}
    $OPT -mem2reg -indvars -loop-rotate -f -S "${f%.*}${IRFILESUFFIX}" -o "${f%.*}${BCFILESUFFIX}"
    $OPT -load 	$LOOPPATH $LOOP "${f%.*}${BCFILESUFFIX}" "${LOGDIR}${STRIPPEDNAME%.*}_L$LOGSUFFIX"
    $OPT -load 	$DSPATH $DS "${f%.*}${BCFILESUFFIX}" "${LOGDIR}${STRIPPEDNAME%.*}_CC$LOGSUFFIX"
    $OPT -load 	$COUNTCALLSPATH $COUNTCALLS "${f%.*}${BCFILESUFFIX}" "${LOGDIR}${STRIPPEDNAME%.*}_CALL$LOGSUFFIX"
    $OPT -load  $INSTRUMENTPATH $INSTRUMENT "${f%.*}${IRFILESUFFIX}" -o instrumented_bin
    $LLVM_PATH/bin/lli ./instrumented_bin > "${LOGDIR}${STRIPPEDNAME%.*}_DYN$LOGSUFFIX"
    python3 ../ranker.py "${LOGDIR}${STRIPPEDNAME%.*}_L$LOGSUFFIX" "${LOGDIR}${STRIPPEDNAME%.*}_CC$LOGSUFFIX" "${LOGDIR}${STRIPPEDNAME%.*}_CALL$LOGSUFFIX" "${LOGDIR}${STRIPPEDNAME%.*}_DYN$LOGSUFFIX" > "${LOGDIR}${STRIPPEDNAME%.*}_AnalysisResults$LOGSUFFIX"
done

echo "Check $LOGDIR"
