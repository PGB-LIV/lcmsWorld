/*
Copyright 2017, Michael R. Hoopmann, Institute for Systems Biology
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _MZIMLSTRUCTS_H
#define _MZIMLSTRUCTS_H

#include <cstdio>
#include <string>

enum mzidElement{
  AdditionalSearchParams,
  AnalysisCollection,
  AnalysisData,
  AnalysisProtocolCollection,
  AnalysisSoftware,
  AnalysisSoftwareList,
  CvList,
  DBSequence,
  DataCollection,
  DatabaseName,
  FileFormat,
  Inputs,
  Modification,
  ModificationParams,
  MzIdentML,
  Peptide,
  PeptideEvidence,
  PeptideHypothesis,
  PeptideSequence,
  ProteinAmbiguityGroup,
  ProteinDetection,
  ProteinDetectionHypothesis,
  ProteinDetectionList,
  ProteinDetectionProtocol,
  SearchDatabase,
  SearchModification,
  SearchType,
  SequenceCollection,
  SoftwareName,
  SpecificityRules,
  SpectraData,
  SpectrumIDFormat,
  SpectrumIdentification,
  SpectrumIdentificationItem,
  SpectrumIdentificationList,
  SpectrumIdentificationProtocol,
  SpectrumIdentificationResult,
  Threshold
};

typedef struct sCvParam{
  std::string accession;
  std::string cvRef;
  std::string name;
  std::string unitAccession;
  std::string unitCvRef;
  std::string unitName;
  std::string value;
  sCvParam(){
    accession = "null";
    cvRef = "null";
    name = "null";
    unitAccession.clear();
    unitCvRef.clear();
    unitName.clear();
    value.clear();
  }
  bool operator==(const sCvParam& p){
    if (accession.compare(p.accession)!=0) return false;
    if (cvRef.compare(p.cvRef) != 0) return false;
    if (name.compare(p.name) != 0) return false;
    if (unitAccession.compare(p.unitAccession) != 0) return false;
    if (unitCvRef.compare(p.unitCvRef) != 0) return false;
    if (unitName.compare(p.unitName) != 0) return false;
    if (value.compare(p.value) != 0) return false;
    return true;
  }
  bool operator!=(const sCvParam& s){ return !operator==(s); }
  void writeOut(FILE* f, int tabs = -1){
    if (accession.compare("null")==0) return;
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<cvParam accession=\"%s\" name=\"%s\" cvRef=\"%s\"", &accession[0],&name[0],&cvRef[0]);
    if (unitAccession.size()>0) fprintf(f, " unitAccession=\"%s\"", &unitAccession[0]);
    if (unitCvRef.size()>0) fprintf(f, " unitCvRef=\"%s\"", &unitCvRef[0]);
    if (unitName.size()>0) fprintf(f, " unitName=\"%s\"", &unitName[0]);
    if (value.size()>0) fprintf(f, " value=\"%s\"", &value[0]);
    fprintf(f, "/>\n");
  }
} sCvParam;

typedef struct sUserParam{
  std::string name;
  std::string type;
  std::string unitAccession;
  std::string unitCvRef;
  std::string unitName;
  std::string value;
  sUserParam(){
    name = "null";
    type.clear();
    unitAccession.clear();
    unitCvRef.clear();
    unitName.clear();
    value.clear();
  }
  bool operator==(const sUserParam& p){
    if (type.compare(p.type) != 0) return false;
    if (name.compare(p.name) != 0) return false;
    if (unitAccession.compare(p.unitAccession) != 0) return false;
    if (unitCvRef.compare(p.unitCvRef) != 0) return false;
    if (unitName.compare(p.unitName) != 0) return false;
    if (value.compare(p.value) != 0) return false;
    return true;
  }
  bool operator!=(const sUserParam& s){ return !operator==(s); }
  void writeOut(FILE* f, int tabs = -1){
    if (name.compare("null") == 0) return;
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<userParam name=\"%s\"",&name[0]);
    if(type.size()>0) fprintf(f, " type=\"%s\"", &type[0]);
    if (unitAccession.size()>0) fprintf(f, " unitAccession=\"%s\"", &unitAccession[0]);
    if (unitCvRef.size()>0) fprintf(f, " unitCvRef=\"%s\"", &unitCvRef[0]);
    if (unitName.size()>0) fprintf(f, " unitName=\"%s\"", &unitName[0]);
    if (value.size()>0) fprintf(f, " value=\"%s\"", &value[0]);
    fprintf(f, "/>\n");
  }
} sUserParam;

typedef struct sCustomizations{
  std::string text;
  void writeOut(FILE* f, int tabs = -1){
    if (text.size()==0) return;
    int i;
    for (i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f,"<Customizations>\n");
    for (i = 0; i<tabs+1; i++) fprintf(f, " ");
    fprintf(f,"%s\n",&text[0]);
    for (i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "</Customizations>\n");
  }
} sCustomizations;

typedef struct sCV{
  std::string fullName;
  std::string id;
  std::string uri;
  std::string version;
  void writeOut(FILE* f, int tabs = -1){
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<cv id=\"%s\" fullName=\"%s\" uri=\"%s\"", &id[0], &fullName[0], &uri[0]);
    if (version.size()>0) fprintf(f, " version=\"%s\"", &version[0]);
    fprintf(f, "/>\n");
  }
} sCV;

typedef struct sInputSpectra{
  std::string spectraDataRef;
  bool operator==(const sInputSpectra& s){
    if (spectraDataRef.compare(s.spectraDataRef)!=0) return false;
    return true;
  }
  void writeOut(FILE* f, int tabs = -1){
    if (spectraDataRef.size() == 0) return;
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<InputSpectra spectraData_ref=\"%s\"/>\n",&spectraDataRef[0]);
  }
} sInputSpectra;

typedef struct sSearchDatabaseRef{
  std::string searchDatabaseRef;
  bool operator==(const sSearchDatabaseRef& s){
    if (searchDatabaseRef.compare(s.searchDatabaseRef) != 0) return false;
    return true;
  }
  void writeOut(FILE* f, int tabs = -1){
    if (searchDatabaseRef.size() == 0) return;
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<SearchDatabaseRef searchDatabase_ref=\"%s\"/>\n", &searchDatabaseRef[0]);
  }
} sSearchDatabaseRef;

typedef struct sInputSpectrumIdentifications{
  std::string spectrumIdentificationListRef;
  void writeOut(FILE* f, int tabs = -1){
    if (spectrumIdentificationListRef.size() == 0) return;
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<InputSpectrumIdentifications spectrumIdentificationList_ref=\"%s\"/>\n", &spectrumIdentificationListRef[0]);
  }
} sInputSpectrumIdentifications;

typedef struct sExternalFormatDocumentation{
  std::string text;
  void writeOut(FILE* f, int tabs = -1){
    if (text.size() == 0) return;
    int i;
    for (i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<ExternalFormatDocumentation>\n");
    for (i = 0; i<tabs + 1; i++) fprintf(f, " ");
    fprintf(f, "%s\n", &text[0]);
    for (i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "</ExternalFormatDocumentation>\n");
  }
} sExternalFormatDocumentation;

typedef struct sPeptideEvidenceRef{
  std::string peptideEvidenceRef;
  void writeOut(FILE* f, int tabs = -1){
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<PeptideEvidenceRef peptideEvidence_ref=\"%s\"/>\n", &peptideEvidenceRef[0]);
  }
} sPeptideEvidenceRef;

typedef struct sSeq{
  std::string text;
  void writeOut(FILE* f, int tabs = -1){
    if (text.size() == 0) return;
    int i;
    for (i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<Seq>%s</Seq>\n",&text[0]);
  }
} sSeq;

typedef struct sPeptideSequence{
  std::string text;
  bool operator==(const sPeptideSequence& s){
    if (text.compare(s.text) != 0) return false;
    return true;
  }
  void writeOut(FILE* f, int tabs = -1){
    if (text.size() == 0) return;
    int i;
    for (i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<PeptideSequence>%s</PeptideSequence>\n", &text[0]);
  }
} sPeptideSequence;

typedef struct sSpectrumIdentificationItemRef{
  std::string text;
  sSpectrumIdentificationItemRef(){
    text = "null";
  }
  void writeOut(FILE* f, int tabs = -1){
    if (text.size() == 0) return;
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<SpectrumIdentificationItemRef spectrumIdentificationItem_ref=\"%s\"/>\n", &text[0]);
  }
} sSpectrumIdentificationItemRef;

typedef struct sSubstitutionModification{
  double avgMassDelta;
  int location;
  double monoisotopicMassDelta;
  std::string originalResidue;
  std::string replacementResidue;
  bool operator==(const sSubstitutionModification& m){
    if (avgMassDelta!=m.avgMassDelta) return false;
    if (location!=m.location) return false;
    if (monoisotopicMassDelta!=m.monoisotopicMassDelta) return false;
    if (originalResidue.compare(m.originalResidue)!=0) return false;
    if (replacementResidue.compare(m.replacementResidue)!=0) return false;
    return true;
  }
  void writeOut(FILE* f, int tabs = -1){
    for (int i = 0; i<tabs; i++) fprintf(f, " ");
    fprintf(f, "<SubstitutionModification");
    if(avgMassDelta>0) fprintf(f," avgMassDelta=\"%.6lf\"",avgMassDelta);
    if (location>-1) fprintf(f, " location=\"%d\"", location); 
    if (monoisotopicMassDelta>0) fprintf(f, " monoisotopicMassDelta=\"%.6lf\"", monoisotopicMassDelta); 
    fprintf(f, " originalResidue=\"%s\" replacementResidue=\"%s\"/>\n",&originalResidue[0],&replacementResidue[0]);
  }
} sSubstitutionModification;

typedef struct sXRefSIIPE{
  int charge;
  std::string spectrumIdentificationItemRef;
  std::string peptideEvidenceRef;
} sXRefSIIPE;

typedef struct sPepTable{
  size_t index;
  std::string seq;
} sPepTable;

typedef struct sXLPepTable{
  std::string ID;
  std::string refLong;
  std::string refShort;
  std::string value;
} sXLPepTable;

#endif