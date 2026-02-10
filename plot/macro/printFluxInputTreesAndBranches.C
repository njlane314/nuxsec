#include "TBranch.h"
#include "TClass.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TKey.h"
#include "TObjArray.h"
#include "TString.h"
#include "TTree.h"

#include <cstdio>
#include <map>

namespace {

void print_indent(const int level) {
  for (int i = 0; i < level; ++i) {
    std::printf("  ");
  }
}

void print_branch_list(const TObjArray* branches, const int indent_level) {
  if (branches == NULL) {
    return;
  }

  for (int i = 0; i < branches->GetEntries(); ++i) {
    TBranch* branch = dynamic_cast<TBranch*>(branches->At(i));
    if (branch == NULL) {
      continue;
    }

    print_indent(indent_level);
    std::printf("- %s\n", branch->GetName());
    print_branch_list(branch->GetListOfBranches(), indent_level + 1);
  }
}

void print_tree(const TString& object_path, TTree* tree) {
  if (tree == NULL) {
    return;
  }

  std::printf("\nTree: %s\n", object_path.Data());
  std::printf("  Entries: %lld\n", static_cast<Long64_t>(tree->GetEntries()));
  std::printf("  Branches:\n");
  print_branch_list(tree->GetListOfBranches(), 2);
}

void scan_directory(TDirectory* directory,
                    const TString& directory_path,
                    int& tree_count,
                    int& object_count,
                    std::map<TString, int>& class_counts) {
  if (directory == NULL) {
    return;
  }

  TIter next_key(directory->GetListOfKeys());
  TKey* key = NULL;
  while ((key = dynamic_cast<TKey*>(next_key())) != NULL) {
    TClass* key_class = TClass::GetClass(key->GetClassName());
    const bool is_tree_key = key_class != NULL && key_class->InheritsFrom(TTree::Class());
    const bool is_directory_key = key_class != NULL && key_class->InheritsFrom(TDirectory::Class());

    TObject* object = key->ReadObj();
    if (object == NULL) {
      continue;
    }

    TString object_path = directory_path;
    if (!object_path.EndsWith("/")) {
      object_path += "/";
    }
    object_path += key->GetName();

    ++object_count;
    class_counts[key->GetClassName()] += 1;

    if (is_tree_key || object->InheritsFrom(TTree::Class())) {
      TTree* tree = dynamic_cast<TTree*>(object);
      print_tree(object_path, tree);
      ++tree_count;
      continue;
    }

    if (is_directory_key || object->InheritsFrom(TDirectory::Class())) {
      TDirectory* sub_directory = dynamic_cast<TDirectory*>(object);
      scan_directory(sub_directory, object_path, tree_count, object_count, class_counts);
    }
  }
}

void print_class_summary(const std::map<TString, int>& class_counts) {
  std::printf("Object types:\n");
  for (std::map<TString, int>::const_iterator it = class_counts.begin(); it != class_counts.end(); ++it) {
    std::printf("  - %s: %d\n", it->first.Data(), it->second);
  }
}

void print_file_tree_summary(const char* file_path) {
  if (file_path == NULL || file_path[0] == '\0') {
    std::printf("[printFluxInputTreesAndBranches] empty file path provided\n");
    return;
  }

  TFile input_file(file_path, "READ");
  if (input_file.IsZombie()) {
    std::printf("[printFluxInputTreesAndBranches] failed to open file: %s\n", file_path);
    return;
  }

  std::printf("\n============================================================\n");
  std::printf("File: %s\n", file_path);
  std::printf("============================================================\n");

  int tree_count = 0;
  int object_count = 0;
  std::map<TString, int> class_counts;
  scan_directory(&input_file, input_file.GetName(), tree_count, object_count, class_counts);

  std::printf("\n");
  print_class_summary(class_counts);
  std::printf("\nSummary: objects=%d trees=%d\n", object_count, tree_count);

  if (tree_count == 0) {
    std::printf("No TTrees were found in this file.\n");
  }
}

} // namespace

void printFluxInputTreesAndBranches(
  const char* fhc_file = "/exp/uboone/data/users/bnayak/ppfx/flugg_studies/NuMIFlux_dk2nu_FHC.root",
  const char* rhc_file = "/exp/uboone/data/users/bnayak/ppfx/flugg_studies/NuMIFlux_dk2nu_RHC.root"
) {
  print_file_tree_summary(fhc_file);
  print_file_tree_summary(rhc_file);
}
