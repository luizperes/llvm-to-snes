; RUN: llc -mtriple=x86_64-pc-linux -x86-cmov-converter=true -verify-machineinstrs < %s | FileCheck %s

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; This test checks that x86-cmov-converter optimization transform CMOV
;; instruction into branches when it is profitable.
;; There are 5 cases below:
;;   1. CmovInCriticalPath:
;;        CMOV depends on the condition and it is in the hot path.
;;        Thus, it worths transforming.
;;
;;   2. CmovNotInCriticalPath:
;;        similar test like in (1), just that CMOV is not in the hot path.
;;        Thus, it does not worth transforming.
;;
;;   3. MaxIndex:
;;        Maximum calculation algorithm that is looking for the max index,
;;        calculating CMOV value is cheaper than calculating CMOV condition.
;;        Thus, it worths transforming.
;;
;;   4. MaxValue:
;;        Maximum calculation algorithm that is looking for the max value,
;;        calculating CMOV value is not cheaper than calculating CMOV condition.
;;        Thus, it does not worth transforming.
;;
;;   5. BinarySearch:
;;        Usually, binary search CMOV is not predicted.
;;        Thus, it does not worth transforming.
;;
;; Test was created using the following command line:
;; > clang -S -O2 -m64 -fno-vectorize -fno-unroll-loops -emit-llvm foo.c -o -
;; Where foo.c is:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;void CmovInHotPath(int n, int a, int b, int *c, int *d) {
;;  for (int i = 0; i < n; i++) {
;;    int t = c[i];
;;    if (c[i] * a > b)
;;      t = 10;
;;    c[i] = t;
;;  }
;;}
;;
;;
;;void CmovNotInHotPath(int n, int a, int b, int *c, int *d) {
;;  for (int i = 0; i < n; i++) {
;;    int t = c[i];
;;    if (c[i] * a > b)
;;      t = 10;
;;    c[i] = t;
;;    d[i] /= b;
;;  }
;;}
;;
;;
;;int MaxIndex(int n, int *a) {
;;  int t = 0;
;;  for (int i = 1; i < n; i++) {
;;    if (a[i] > a[t])
;;      t = i;
;;  }
;;  return a[t];
;;}
;;
;;
;;int MaxValue(int n, int *a) {
;;  int t = a[0];
;;  for (int i = 1; i < n; i++) {
;;    if (a[i] > t)
;;      t = a[i];
;;  }
;;  return t;
;;}
;;
;;typedef struct Node Node;
;;struct Node {
;;  unsigned Val;
;;  Node *Right;
;;  Node *Left;
;;};
;;
;;unsigned BinarySearch(unsigned Mask, Node *Curr, Node *Next) {
;;  while (Curr->Val > Next->Val) {
;;    Curr = Next;
;;    if (Mask & (0x1 << Curr->Val))
;;      Next = Curr->Right;
;;    else
;;      Next = Curr->Left;
;;  }
;;  return Curr->Val;
;;}
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%struct.Node = type { i32, %struct.Node*, %struct.Node* }

; CHECK-LABEL: CmovInHotPath
; CHECK-NOT: cmov
; CHECK: jg

define void @CmovInHotPath(i32 %n, i32 %a, i32 %b, i32* nocapture %c, i32* nocapture readnone %d) #0 {
entry:
  %cmp14 = icmp sgt i32 %n, 0
  br i1 %cmp14, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  ret void

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds i32, i32* %c, i64 %indvars.iv
  %0 = load i32, i32* %arrayidx, align 4
  %mul = mul nsw i32 %0, %a
  %cmp3 = icmp sgt i32 %mul, %b
  %. = select i1 %cmp3, i32 10, i32 %0
  store i32 %., i32* %arrayidx, align 4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.cond.cleanup, label %for.body
}

; CHECK-LABEL: CmovNotInHotPath
; CHECK: cmovg

define void @CmovNotInHotPath(i32 %n, i32 %a, i32 %b, i32* nocapture %c, i32* nocapture %d) #0 {
entry:
  %cmp18 = icmp sgt i32 %n, 0
  br i1 %cmp18, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  ret void

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds i32, i32* %c, i64 %indvars.iv
  %0 = load i32, i32* %arrayidx, align 4
  %mul = mul nsw i32 %0, %a
  %cmp3 = icmp sgt i32 %mul, %b
  %. = select i1 %cmp3, i32 10, i32 %0
  store i32 %., i32* %arrayidx, align 4
  %arrayidx7 = getelementptr inbounds i32, i32* %d, i64 %indvars.iv
  %1 = load i32, i32* %arrayidx7, align 4
  %div = sdiv i32 %1, %b
  store i32 %div, i32* %arrayidx7, align 4
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.cond.cleanup, label %for.body
}

; CHECK-LABEL: MaxIndex
; CHECK-NOT: cmov
; CHECK: jg

define i32 @MaxIndex(i32 %n, i32* nocapture readonly %a) #0 {
entry:
  %cmp14 = icmp sgt i32 %n, 1
  br i1 %cmp14, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.cond.cleanup.loopexit:                        ; preds = %for.body
  %phitmp = sext i32 %i.0.t.0 to i64
  br label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit, %entry
  %t.0.lcssa = phi i64 [ 0, %entry ], [ %phitmp, %for.cond.cleanup.loopexit ]
  %arrayidx5 = getelementptr inbounds i32, i32* %a, i64 %t.0.lcssa
  %0 = load i32, i32* %arrayidx5, align 4
  ret i32 %0

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 1, %for.body.preheader ]
  %t.015 = phi i32 [ %i.0.t.0, %for.body ], [ 0, %for.body.preheader ]
  %arrayidx = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  %1 = load i32, i32* %arrayidx, align 4
  %idxprom1 = sext i32 %t.015 to i64
  %arrayidx2 = getelementptr inbounds i32, i32* %a, i64 %idxprom1
  %2 = load i32, i32* %arrayidx2, align 4
  %cmp3 = icmp sgt i32 %1, %2
  %3 = trunc i64 %indvars.iv to i32
  %i.0.t.0 = select i1 %cmp3, i32 %3, i32 %t.015
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.cond.cleanup.loopexit, label %for.body
}

; CHECK-LABEL: MaxValue
; CHECK-NOT: jg
; CHECK: cmovg

define i32 @MaxValue(i32 %n, i32* nocapture readonly %a) #0 {
entry:
  %0 = load i32, i32* %a, align 4
  %cmp13 = icmp sgt i32 %n, 1
  br i1 %cmp13, label %for.body.preheader, label %for.cond.cleanup

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %n to i64
  br label %for.body

for.cond.cleanup:                                 ; preds = %for.body, %entry
  %t.0.lcssa = phi i32 [ %0, %entry ], [ %.t.0, %for.body ]
  ret i32 %t.0.lcssa

for.body:                                         ; preds = %for.body.preheader, %for.body
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body ], [ 1, %for.body.preheader ]
  %t.014 = phi i32 [ %.t.0, %for.body ], [ %0, %for.body.preheader ]
  %arrayidx1 = getelementptr inbounds i32, i32* %a, i64 %indvars.iv
  %1 = load i32, i32* %arrayidx1, align 4
  %cmp2 = icmp sgt i32 %1, %t.014
  %.t.0 = select i1 %cmp2, i32 %1, i32 %t.014
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.cond.cleanup, label %for.body
}

; CHECK-LABEL: BinarySearch
; CHECK: cmov

define i32 @BinarySearch(i32 %Mask, %struct.Node* nocapture readonly %Curr, %struct.Node* nocapture readonly %Next) #0 {
entry:
  %Val8 = getelementptr inbounds %struct.Node, %struct.Node* %Curr, i64 0, i32 0
  %0 = load i32, i32* %Val8, align 8
  %Val19 = getelementptr inbounds %struct.Node, %struct.Node* %Next, i64 0, i32 0
  %1 = load i32, i32* %Val19, align 8
  %cmp10 = icmp ugt i32 %0, %1
  br i1 %cmp10, label %while.body, label %while.end

while.body:                                       ; preds = %entry, %while.body
  %2 = phi i32 [ %4, %while.body ], [ %1, %entry ]
  %Next.addr.011 = phi %struct.Node* [ %3, %while.body ], [ %Next, %entry ]
  %shl = shl i32 1, %2
  %and = and i32 %shl, %Mask
  %tobool = icmp eq i32 %and, 0
  %Left = getelementptr inbounds %struct.Node, %struct.Node* %Next.addr.011, i64 0, i32 2
  %Right = getelementptr inbounds %struct.Node, %struct.Node* %Next.addr.011, i64 0, i32 1
  %Left.sink = select i1 %tobool, %struct.Node** %Left, %struct.Node** %Right
  %3 = load %struct.Node*, %struct.Node** %Left.sink, align 8
  %Val1 = getelementptr inbounds %struct.Node, %struct.Node* %3, i64 0, i32 0
  %4 = load i32, i32* %Val1, align 8
  %cmp = icmp ugt i32 %2, %4
  br i1 %cmp, label %while.body, label %while.end

while.end:                                        ; preds = %while.body, %entry
  %.lcssa = phi i32 [ %0, %entry ], [ %2, %while.body ]
  ret i32 %.lcssa
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; The following test checks that x86-cmov-converter optimization transforms
;; CMOV instructions into branch correctly.
;;
;; MBB:
;;   cond = cmp ...
;;   v1 = CMOVgt t1, f1, cond
;;   v2 = CMOVle s1, f2, cond
;;
;; Where: t1 = 11, f1 = 22, f2 = a
;;
;; After CMOV transformation
;; -------------------------
;; MBB:
;;   cond = cmp ...
;;   ja %SinkMBB
;;
;; FalseMBB:
;;   jmp %SinkMBB
;;
;; SinkMBB:
;;   %v1 = phi[%f1, %FalseMBB], [%t1, %MBB]
;;   %v2 = phi[%f1, %FalseMBB], [%f2, %MBB] ; For CMOV with OppCC switch
;;                                          ; true-value with false-value
;;                                          ; Phi instruction cannot use
;;                                          ; previous Phi instruction result
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; CHECK-LABEL: Transform
; CHECK-NOT: cmov
; CHECK:         divl    [[a:%[0-9a-z]*]]
; CHECK:         cmpl    [[a]], %eax
; CHECK:         movl    $11, [[s1:%[0-9a-z]*]]
; CHECK:         movl    [[a]], [[s2:%[0-9a-z]*]]
; CHECK:         ja      [[SinkBB:.*]]
; CHECK: [[FalseBB:.*]]:
; CHECK:         movl    $22, [[s1]]
; CHECK:         movl    $22, [[s2]]
; CHECK: [[SinkBB]]:
; CHECK:         ja

define void @Transform(i32 *%arr, i32 *%arr2, i32 %a, i32 %b, i32 %c, i32 %n) #0 {
entry:
  %cmp10 = icmp ugt i32 0, %n
  br i1 %cmp10, label %while.body, label %while.end

while.body:                                       ; preds = %entry, %while.body
  %i = phi i32 [ %i_inc, %while.body ], [ 0, %entry ]
  %arr_i = getelementptr inbounds i32, i32* %arr, i32 %i
  %x = load i32, i32* %arr_i, align 4
  %div = udiv i32 %x, %a
  %cond = icmp ugt i32 %div, %a
  %condOpp = icmp ule i32 %div, %a
  %s1 = select i1 %cond, i32 11, i32 22
  %s2 = select i1 %condOpp, i32 %s1, i32 %a
  %sum = urem i32 %s1, %s2
  store i32 %sum, i32* %arr_i, align 4
  %i_inc = add i32 %i, 1
  %cmp = icmp ugt i32 %i_inc, %n
  br i1 %cmp, label %while.body, label %while.end

while.end:                                        ; preds = %while.body, %entry
  ret void
}

attributes #0 = {"target-cpu"="x86-64"}
